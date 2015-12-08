var $bios:BiosAPI;

var __bios__ = function (api) {
  //Remove global bios reference
  __bios__ = null;

  class BiosComponentApiImpl implements BiosComponentApi {
    list(filter?:string):{ [key:string]:string } {
      return api.component.list(filter || "");
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
        if (method.getter || method.setter) {
          Object.defineProperty(res, method.name, {
            enumerable: true,
            "get": method.getter ? ((n) => api.component.invoke(address, n, [])).bind(this, method.name) : null,
            "set": method.setter ? ((n, v) => api.component.invoke(address, n, [v])).bind(this, method.name) : null
          });
        } else {
          Object.defineProperty(res, method.name, {
            enumerable: true,
            writable: false,
            value: ((n, ...args) => api.component.invoke(address, n, args)).bind(this, method.name)
          });
        }
      }
      return res;
    }

    first(type:string):any {
      let components = $bios.component.list(type);
      let address;
      for (let k in components) {
        address = k;
        break;
      }

      if (!address) {
        return null;
      } else {
        return $bios.component.proxy(address);
      }
    }
  }

  class BiosAPIImpl implements BiosAPI {
    component = new BiosComponentApiImpl();

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
  }

  $bios = new BiosAPIImpl();

  let eeprom: EEPROMComponentAPI = $bios.component.first("eeprom");

  if (!eeprom) {
    $bios.crash('No eeprom!');
  } else {
    $bios.compile(eeprom.get());
  }
};
