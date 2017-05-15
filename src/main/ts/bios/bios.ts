import {EEPROMComponentAPI, FilesystemComponentAPI, GPUComponent} from '../os/usr/lib/externalComponents';

//Private interfaces
interface RawBiosAPI {
  component: RawComponentAPI
  computer: RawComputerAPI
  bios: InternalAPI
}

interface RawComponentAPI {
  list(name: string): ComponentInfo[]
  invoke(address: string, name: string, args: any[]): any[]
  doc(address: string, name: string): string
  methods(address: string): { [p: string]: ComponentMethodInfo };
  type(address: string): string;
}

interface RawComputerAPI {
  signal(): Signal | null | undefined;
  sleep(time: number): void;
  address(): string;
  tmpAddress(): string;
  freeMemory(): number
  totalMemory(): number
  energy(): number
  maxEnergy(): number
  uptime(): number
}

interface InternalAPI {
  log(msg: string): void
  crash(msg: string): void
  compile(name: string, script: string): any
}

// Bios impl

global.__bios__ = function (api: RawBiosAPI) {
  //Remove global bios reference
  global.__bios__ = null;
  delete global.__bios__;

  //Also duktape
  let duktape: any = global.Duktape;
  global.Duktape = null;

  // Yielding should only happen in the bios!
  type BiosYieldType = "invoke" | "sleep";
  class BiosYield {
    constructor(public type: BiosYieldType, public arg: number | {
      address: string,
      name: string,
      args: any[]
    }){
    }
  }

  class BiosComponentApiImpl implements BiosComponentApi {
    list(filter?: string | RegExp): ComponentInfo[] {
      let arr = $bios.javaArrayToList(api.component.list(''));
      if (filter) {
        if (filter instanceof RegExp) {
          return arr.filter(v => !!v.type.match(filter));
        } else {
          return arr.filter(v => v.type === filter);
        }
      } else {
        return arr;
      }
    }

    invoke(address: string, name: string, ...args: any[]): any | any[] {
      try {
        //Have to use co-routine magic here just in case
        return $bios.Thread.yeild(new BiosYield("invoke", {
          address: address,
          name: name,
          args: args
        }));
      } catch (e) {
        throw new Error(`Error invoking ${name}: ${e.message}`);
      }
    }

    doc(address: string, name: string): string {
      return api.component.doc(address, name);
    }

    methods(address: string): { [key: string]: ComponentMethodInfo } {
      return api.component.methods(address);
    }

    type(address: string): string {
      return api.component.type(address)
    }

    proxy(address: string): any {
      let res: any = {};
      let methods = this.methods(address);
      if (!methods) return null;
      Object.defineProperty(res, 'uuid', {
        enumerable: true,
        value: address
      });
      for (let name in methods) {
        let method = methods[name];
        let g = (n: string) => () => this.invoke.apply(this, [address, n]);
        let s = (n: string, v: any) => this.invoke.apply(this, [address, n, v]);
        if (method.getter || method.setter) {
          Object.defineProperty(res, method.name, {
            enumerable: true,
            'get': method.getter ? g.bind(this, method.name) : null,
            'set': method.setter ? s.bind(this, method.name) : null
          });
        } else {
          let i = (n: string, ...args: any[]) => this.invoke.apply(this, [address, n].concat(args));
          Object.defineProperty(res, method.name, {
            enumerable: true,
            writable: false,
            value: i.bind(this, method.name)
          });
        }
      }
      return res;
    }

    first(type: string): any {
      let address = $bios.component.list(type)[0].uuid;
      if (!address) {
        return null;
      } else {
        return $bios.component.proxy(address);
      }
    }
  }

  class BiosComputerApiImpl implements BiosComputerApi {
    signal(): Signal {
      let sig = api.computer.signal();
      if (sig) {
        return {
          name: sig.name,
          args: $bios.javaArrayToList(sig.args)
        }
      } else {
        return null;
      }
    }

    sleep(time: number): void {
      $bios.Thread.yeild(new BiosYield('sleep', time));
    }

    address(): string {
      return api.computer.address();
    }

    tmpAddress(): string {
      return api.computer.tmpAddress();
    }

    freeMemory(): number {
      return api.computer.freeMemory();
    }

    totalMemory(): number {
      return api.computer.totalMemory();
    }

    energy(): number {
      return api.computer.energy();
    }

    maxEnergy(): number {
      return api.computer.maxEnergy();
    }

    uptime(): number {
      return api.computer.uptime();
    }

  }

  class BiosApiImpl implements BiosApi {
    component = new BiosComponentApiImpl();
    computer = new BiosComputerApiImpl();
    //Set by the bootloader
    bootFS: FilesystemComponentAPI;
    Thread = duktape.Thread as (typeof Thread);

    kernelThread: Thread;

    crash(msg: string): void {
      let gpu: GPUComponent = $bios.component.first('gpu');
      let screen = $bios.component.first('screen');
      if (gpu && screen) {
        gpu.bind(screen.uuid);
      }
      api.bios.crash(msg);
    }

    compile(filename: string, script: string): any {
      return api.bios.compile(filename, script);
    }

    log(message: string) {
      api.bios.log(message);
    }

    javaArrayToList<T>(arr: T[]): T[] {
      let res = [];
      for (let i = 0; i < arr.length; i++) {
        res.push(arr[i]);
      }
      return res;
    }

    // To be called from oc-js only!
    // TODO: make available only from js (using native code)
    private __runThreaded__(res?: any): BiosYield {
      let biosYield = Thread.resume(this.kernelThread, res) as BiosYield;
      if (biosYield) {
        return biosYield;
      } else {
        this.crash("Invalid kernel yield; Probably crashed or ended execution (no yield)")
      }
    }
  }

  let biosImpl = new BiosApiImpl();
  global.$bios = biosImpl;

  let eeprom: EEPROMComponentAPI = $bios.component.first('eeprom');

  if (!eeprom) {
    $bios.crash('No eeprom!');
  } else {
    // Load eeprom main
    let eepromSrc = `
    (function(){
      global['__eeprom_main__'] = function() {
        ${eeprom.get()};
      };
    })();`;
    $bios.compile('eeprom', eepromSrc);
    let main = global['__eeprom_main__'];
    delete global['__eeprom_main__'];

    // Set up co-routine
    biosImpl.kernelThread = new $bios.Thread(main)
  }
};
