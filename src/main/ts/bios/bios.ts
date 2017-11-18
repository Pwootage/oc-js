import { EEPROMComponentAPI, FilesystemComponentAPI, GPUComponent } from '../os/usr/lib/externalComponents';

//Private interfaces
type BiosCallType = "invoke" | "sleep";

interface ComponentInvokeParams {
  address: string,
  name: string,
  args: any[]
}

type BiosCallParams = [ComponentInvokeParams] | any[];

interface BiosCall {
  readonly name: string;
  readonly args: BiosCallParams;
}

declare function __biosCall(asyncCall: AsyncBiosCall): void;

const biosCallResultStore = new Map<number, AsyncBiosCall>();
const callID = function*() {
  let id = 0;
  while (true) {
    if (id >= Number.MAX_SAFE_INTEGER - 1) {
      id = 0;
    }
    while (biosCallResultStore.has(id)) {
      id++;
    }
    yield id;
  }
}();

class AsyncBiosCall {
  constructor(call: BiosCall) {
    this.call = call;
    this.id = callID.next().value;
    this.promise = new Promise((resolve, reject) => {
      this.resolve = resolve;
      this.reject = reject;
    });
    // Auto-remove on resolve/reject
    this.promise.then(() => {
      biosCallResultStore.delete(this.id);
    }).catch(() => {
      biosCallResultStore.delete(this.id);
    });
  }
  call: BiosCall;
  id: number;
  promise: Promise<any>
  resolve: (val: any) => void;
  reject: (val: any) => void;
}

function biosCall<T>(call: BiosCall): Promise<T> {
  const asyncCall = new AsyncBiosCall(call);
  biosCallResultStore.set(asyncCall.id, asyncCall);
  __biosCall(asyncCall);
  return asyncCall.promise;
}

class BiosComponentApiImpl implements BiosComponentApi {
  async list(filter?: string | RegExp): Promise<ComponentInfo[]> {
    const arr = await biosCall<ComponentInfo[]>({
      name: "component.list",
      args: ['']
    });
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

  async invoke(address: string, name: string, ...args: any[]): Promise<any | any[]> {
    try {
      //Have to use co-routine magic here just in case
      //TODO: figure this out
    } catch (e) {
      throw new Error(`Error invoking ${name}: ${e.message}`);
    }
  }

  async doc(address: string, name: string): Promise<string> {
    return biosCall<string>({
      name: 'component.doc',
      args: [address, name]
    });
  }

  async methods(address: string): Promise<{ [key: string]: ComponentMethodInfo }> {
    return biosCall<{ [key: string]: ComponentMethodInfo }>({
      name: 'component.methods',
      args: [address]
    });
  }

  async type(address: string): Promise<string> {
    return biosCall<string>({
      name: 'component.type',
      args: [address]
    });
  }

  async proxy(address: string): Promise<any> {
    const res: any = {};
    const methods = await this.methods(address);
    if (!methods) return null;
    Object.defineProperty(res, 'uuid', {
      enumerable: true,
      value: address
    });
    for (const name in methods) {
      const method = methods[name];
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

  async first(type: string): Promise<any> {
    const address = (await this.list())[0].uuid;
    if (!address) {
      return null;
    } else {
      return $bios.component.proxy(address);
    }
  }
}

class BiosComputerApiImpl implements BiosComputerApi {
  async signal(): Promise<Signal | null> {
    const sig = await biosCall<Signal | null | undefined>({
      name: 'computer.signal',
      args: []
    });
    if (sig) {
      return sig;
    } else {
      return null;
    }
  }

  sleep(time: number): Promise<void> {
    return biosCall({
      name: 'computer.sleep',
      args: [time]
    });
  }

  address(): Promise<string> {
    return biosCall({
      name: 'computer.address',
      args: []
    });
  }

  tmpAddress(): Promise<string> {
    return biosCall({
      name: 'computer.tmpAddress',
      args: []
    });
  }

  freeMemory(): Promise<number> {
    return biosCall({
      name: 'computer.freeMemory',
      args: []
    });
  }

  totalMemory(): Promise<number> {
    return biosCall({
      name: 'computer.totalMemory',
      args: []
    });
  }

  energy(): Promise<number> {
    return biosCall({
      name: 'computer.energy',
      args: []
    });
  }

  maxEnergy(): Promise<number> {
    return biosCall({
      name: 'computer.maxEnergy',
      args: []
    });
  }

  uptime(): Promise<number> {
    return biosCall({
      name: 'computer.uptime',
      args: []
    });
  }

}

class BiosApiImpl implements BiosApi {
  component = new BiosComponentApiImpl();
  computer = new BiosComputerApiImpl();
  //Set by the bootloader
  bootFS: FilesystemComponentAPI;

  async crash(msg: string): Promise<void> {
    const gpu: GPUComponent = await $bios.component.first('gpu');
    const screen = await $bios.component.first('screen');
    if (gpu && screen) {
      gpu.bind(screen.uuid);
    }
    return biosCall<void>({
      name: 'bios.crash',
      args: [msg]
    });
  }

  compile(filename: string, script: string): Promise<any> {
    return biosCall({
      name: 'bios.compile',
      args: [filename, script]
    });
  }

  log(message: string) {
    biosCall({
      name: 'bios.log',
      args: [message]
    });
  }

  // To be called from oc-js only!
  // TODO: make available only from js (using native code)
  private __runThreaded__(res?: any) {
    return {
      type: "sleep",
      arg: 100
    };
    //TODO: implement
    // let biosYield = Thread.resume(this.kernelThread, res) as BiosYield;
    // if (biosYield) {
    //   return biosYield;
    // } else {
    //   this.crash("Invalid kernel yield; Probably crashed or ended execution (no yield)")
    // }
  }
}

let biosImpl = new BiosApiImpl();
global.$bios = biosImpl;

global.__main = async function() {
  let eeprom: EEPROMComponentAPI = await $bios.component.first('eeprom');
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
    //TODO: Figure this out
    // biosImpl.kernelThread = new $bios.Thread(main)
  }
}
