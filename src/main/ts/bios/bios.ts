var component:BiosComponentApi;
var crash:(msg:string)=>void;

var __bios__ = function (api) {
  //Remove global bios reference
  __bios__ = null;

  component = {
    list: (filter?:string) =>
      api.component.list(filter || ""),
    invoke: (address:string, name:string, ...args:any[]) =>
      api.component.invoke(address, name, args),
    doc: (address:string, name:string) =>
      api.component.doc(address, name),
    methods: (address:string) => api.component.methods(address),
    type: (address:string) =>
      api.component.type(address),
    proxy: (address:string) => {
      let res:any = {};
      let methods = component.methods(address);
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
    },
    first: (type:string) => {
      let components = component.list(type);
      let address;
      for (let k in components) {
        address = k;
        break;
      }
      if (!address) {
        return null;
      } else {
        return component.proxy(address);
      }
    }
  };

  crash = (msg) => {
    let gpu:GPUComponent = component.first('gpu');
    let screen = component.first('screen');
    if (gpu && screen) {
      gpu.bind(screen.uuid);
    }
    api.console.crash(msg);
  };

  let c = component.list("eeprom");
  let eeprom;
  for (var i in c) {
    eeprom = i;
    break;
  }

  if (!eeprom) {
    crash('No eeprom!');
  }

  while (true) {
    let sig = api.computer.signal();
    if (sig) api.console.log(`${sig.name} ${sig.args}`);
  }
};
