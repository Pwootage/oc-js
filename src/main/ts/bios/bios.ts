///ts:ref=bios.d.ts
/// <reference path="./bios.d.ts"/> ///ts:ref:generated
///ts:ref=component.d.ts
/// <reference path="../components/component.d.ts"/> ///ts:ref:generated

global.__bios__ = function (api) {
  //Remove global bios reference
  global.__bios__ = null;
  delete global.__bios__;

  class BiosComponentApiImpl implements bios.BiosComponentApi {
    list(filter?:string|RegExp):bios.ComponentInfo[] {
      let arr = <bios.ComponentInfo[]>$bios.javaArrayToList(api.component.list(''));
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

    invoke(address:string, name:string, ...args:any[]):any|any[] {
      try {
        let res:any[] = api.component.invoke(address, name, args);
        if (res && res.length === 1) {
          return res[0];
        } else if (res) {
          return $bios.javaArrayToList(res);
        } else {
          return res;
        }
      } catch (e) {
        throw new Error(`Error invoking ${name}: ${e.message}`);
      }
    }

    doc(address:string, name:string):string {
      return api.component.doc(address, name);
    }

    methods(address:string):{ [key:string]:bios.ComponentMethodInfo } {
      return api.component.methods(address);
    }

    type(address:string):string {
      return api.component.type(address)
    }

    proxy(address:string):any {
      let res:any = {};
      let methods = this.methods(address);
      if (!methods) return null;
      Object.defineProperty(res, "uuid", {
        enumerable: true,
        value: address
      });
      for (let name in methods) {
        let method = methods[name];
        let g = (n) => () => this.invoke.apply(this, [address, n]);
        let s = (n, v) => this.invoke.apply(this, [address, n, v]);
        if (method.getter || method.setter) {
          Object.defineProperty(res, method.name, {
            enumerable: true,
            "get": method.getter ? g.bind(this, method.name) : null,
            "set": method.setter ? s.bind(this, method.name) : null
          });
        } else {
          let i = (n, ...args) => this.invoke.apply(this, [address, n].concat(args));
          Object.defineProperty(res, method.name, {
            enumerable: true,
            writable: false,
            value: i.bind(this, method.name)
          });
        }
      }
      return res;
    }

    first(type:string):any {
      let address = $bios.component.list(type)[0].uuid;
      if (!address) {
        return null;
      } else {
        return $bios.component.proxy(address);
      }
    }
  }

  class BiosComputerApiImpl implements bios.BiosComputerApi {
    signal():bios.Signal {
      let sig:bios.Signal = api.computer.signal();
      if (sig) {
        return {
          name: sig.name,
          args: $bios.javaArrayToList(sig.args)
        }
      } else {
        return null;
      }
    }

    sleep(time:number):void {
      api.computer.sleep(time);
    }

    address():string {
      return api.computer.address();
    }

    tmpAddress():string {
      return api.computer.tmpAddress();
    }

    freeMemory():number {
      return api.computer.freeMemory();
    }

    totalMemory():number {
      return api.computer.totalMemory();
    }

    energy():number {
      return api.computer.energy();
    }

    maxEnergy():number {
      return api.computer.maxEnergy();
    }

    uptime():number {
      return api.computer.uptime();
    }

  }

  class BiosApiImpl implements bios.BiosApi {
    component = new BiosComponentApiImpl();
    computer = new BiosComputerApiImpl();
    //Set by the bootloader
    bootFS:component.FilesystemComponentAPI;

    crash(msg:string):void {
      let gpu:component.GPUComponent = $bios.component.first('gpu');
      let screen = $bios.component.first('screen');
      if (gpu && screen) {
        gpu.bind(screen.uuid);
      }
      api.bios.crash(msg);
    }

    compile(filename:string, script:string):any {
      return api.bios.compile(filename, script);
    }

    log(message:string) {
      api.bios.log(message);
    }

    javaArrayToList<T>(arr:T[]):T[] {
      let res = [];
      for (let i = 0; i < arr.length; i++) {
        res.push(arr[i]);
      }
      return res;
    }
  }

  global.$bios = new BiosApiImpl();

  let eeprom:component.EEPROMComponentAPI = $bios.component.first("eeprom");

  if (!eeprom) {
    $bios.crash('No eeprom!');
  } else {
    $bios.compile('eeprom', eeprom.get());
  }
};
