var $bios:BiosApi;

var __bios__ = function (api) {
  //Remove global bios reference
  __bios__ = null;

  class BiosComponentApiImpl implements BiosComponentApi {
    list(filter?:string):ComponentInfo[] {
      return <ComponentInfo[]>$bios.javaArrayToList(api.component.list(filter || ""));
    }

    invoke(address:string, name:string, ...args:any[]):any|any[] {
      let res:any[] = api.component.invoke(address, name, args);
      if (res.length === 1) {
        return res[0];
      } else {
        return res;
      }
    }

    doc(address:string, name:string):string {
      return api.component.doc(address, name);
    }

    methods(address:string):{ [key:string]:ComponentMethodInfo } {
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

  class BiosComputerApiImpl implements BiosComputerApi {
    signal():Signal {
      let sig:Signal = api.computer.signal();
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

  class BiosApiImpl implements BiosApi {
    component = new BiosComponentApiImpl();
    computer = new BiosComputerApiImpl();

    crash(msg:string):void {
      let gpu:GPUComponent = $bios.component.first('gpu');
      let screen = $bios.component.first('screen');
      if (gpu && screen) {
        gpu.bind(screen.uuid);
      }
      api.bios.crash(msg);
    }

    compile(script:string):any {
      return api.bios.compile(script);
    }

    javaArrayToList<T>(arr:T[]):T[] {
      let res = [];
      for (let i = 0; i < arr.length; i++) {
        res.push(arr[i]);
      }
      api.bios.log('res' + res);
      return res;
    }
  }

  $bios = new BiosApiImpl();

  let eeprom:EEPROMComponentAPI = $bios.component.first("eeprom");

  if (!eeprom) {
    $bios.crash('No eeprom!');
  } else {
    api.bios.log('FS' + $bios.component.list('filesystem'));
    $bios.compile(eeprom.get());
  }
};
