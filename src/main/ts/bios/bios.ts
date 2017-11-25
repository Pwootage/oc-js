import { EEPROMComponentAPI, FilesystemComponentAPI, GPUComponent } from '../os/usr/lib/externalComponents';

// Set up global ref
Object.defineProperty(global, 'global', {
  enumerable: false,
  value: global,
  writable: false,
  configurable: false
})

//Private interfaces
interface ComponentInvokeParams {
  address: string,
  name: string,
  args: any[]
}

interface CompileResult {
  state: 'success' | 'error';
}

type BiosCallParams = [ComponentInvokeParams] | any[];

interface BiosCall {
  readonly name: string;
  readonly args: BiosCallParams;
}

interface BiosCallToJava extends BiosCall {
  readonly id: string;
}

interface BiosCallResult {
  readonly state: 'error' | 'sync' | 'async' | 'noop';
  readonly value: any;
  readonly id: string;
}

interface BiosResult {
  id: string;
  name: string;
  value: any
}

type BiosYield = BiosYieldSleep | BiosYieldCall;

interface BiosYieldCall {
  type: 'call';
  arg: BiosCallToJava;
}

interface BiosYieldSleep {
  type: 'sleep';
  arg: number;
}

declare function __bios_call(call: BiosCallToJava): string;

const biosCallResultStore = new Map<string, AsyncBiosCall>();
const callID = function*() {
  let id = 0;
  while (true) {
    if (id >= Number.MAX_SAFE_INTEGER - 1) {
      id = 0;
    }
    id++;
    while (biosCallResultStore.has(id.toFixed(0))) {
      id++;
    }
    yield id.toFixed(0);
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
      $bios.log(`Resolved ${this.call.name}/${this.id}/${JSON.stringify(this.call.args)}`)
      biosCallResultStore.delete(this.id);
    }).catch(() => {
      biosCallResultStore.delete(this.id);
    });
  }
  call: BiosCall;
  id: string;
  promise: Promise<any>
  resolve: (val: any) => void;
  reject: (val: any) => void;
}

function biosCall<T>(call: BiosCall): Promise<T> {
  const asyncCall = new AsyncBiosCall(call);
  biosCallResultStore.set(asyncCall.id, asyncCall);
  const biosResJson = __bios_call({
    ...call,
    id: asyncCall.id
  });
  const result: BiosCallResult = JSON.parse(biosResJson);
  handleBiosCallResult(asyncCall, result);
  return asyncCall.promise;
}

function handleBiosCallResult(asyncCall: AsyncBiosCall, result: BiosCallResult) {
  if (result.state == 'error') {
    __biosError(result.value);
  } else if (result.state == 'sync') {
    // $bios.log(`Sync result ${JSON.stringify(result)}`);
    __biosReturn({
        id: asyncCall.id,
        name: asyncCall.call.name,
        value: result.value
      });
  } else if (result.state == 'noop') {
    // Intentionally do nothing
  } else {
    // Async will be called eventually(tm)
  }
}

function __biosReturn(res: BiosResult) {
  const biosCallback = biosCallResultStore.get(res.id);
  if (biosCallback) {
    biosCallback.resolve(res.value);
  } else {
    $bios.crash(`Couldn't find bios callback for call ${res.name}#${res.id}`);
  }
};
Object.defineProperty(global, '__biosReturn', {
  enumerable: false,
  value: __biosReturn,
  writable: false,
  configurable: false
});

function __biosError(error: string) {
  $bios.crash(`Bios error: ${error}`);
}
Object.defineProperty(global, '__biosError', {
  enumerable: false,
  value: __biosError,
  writable: false,
  configurable: false
});

function __biosRunThreaded(signal: Signal | null, result: BiosCallResult | null): BiosYield {
  if (result) {
    const asyncCall = biosCallResultStore.get(result.id);
    if (asyncCall) {
      handleBiosCallResult(asyncCall, result);
    } else {
      $bios.crash(`Unhandled response in __biosRunThreaded: ${result.id}/${JSON.stringify(result)}`);
    }
  }

  if (signal) {
    $bios.signals.emit('signal', signal);
    $bios.signals.emit(signal.name, ...signal.args);
  }

  // Re-do any outstanding request
  const outstandingRequests = [...biosCallResultStore.entries()];
  if (outstandingRequests.length > 0) {
    const [id, asyncCall] = outstandingRequests[0];
    return {
      type: 'call',
      arg: {
        ...asyncCall.call,
        id
      }
    };
  } else {
    return {
      type: 'sleep',
      arg: 0.05
    }
  }
}
Object.defineProperty(global, '__biosRunThreaded', {
  enumerable: false,
  value: __biosRunThreaded,
  writable: false,
  configurable: false
});

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
    return biosCall<any[]>({
      name: 'component.invoke',
      args: [address, name, args]
    }).then(res => {
      // $bios.log('Component invoke res' + JSON.stringify(res));
      if (res.length == 0) {
        return;
      } else if (res.length == 1) {
        return res[0];
      } else {
        return res;
      }
    });
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

  async proxy<T>(address: string): Promise<T | null> {
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

  async first<T>(type: string): Promise<T | null> {
    const list = await this.list(type);
    const address = list[0] && list[0].uuid;
    if (!address) {
      return null;
    } else {
      return $bios.component.proxy<T>(address);
    }
  }
}

class BiosComputerApiImpl implements BiosComputerApi {
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

type DeregFunction = () => void;
type EventHandler = (...args: any[]) => void;

class EventEmitter {
  private listeners = new Map<string, EventHandler[]>();

  constructor() {}

  on(name:string, fn: EventHandler): DeregFunction {
    let arr = this.listeners.get(name) || [];
    arr.push(fn);
    this.listeners.set(name, arr);
    return this.deregister.bind(this, name, fn);
  }

  emit(name:string, ...args: any[]): void {
    (this.listeners.get(name) || []).forEach(v => v(...args));
  }

  deregister(name: string, fn: EventHandler): void {
    let arr = this.listeners.get(name) || [];
    arr = arr.filter(v => v !== fn);
    if (arr.length == 0) {
      this.listeners.delete(name);
    } else {
      this.listeners.set(name, arr);
    }
  }
}
global.EventEmitter = EventEmitter;

class BiosApiImpl implements BiosApi {
  component = new BiosComponentApiImpl();
  computer = new BiosComputerApiImpl();
  //Set by the bootloader
  bootFS: FilesystemComponentAPI;
  signals = new EventEmitter();

  async crash(msg: string): Promise<void> {
    const gpu = await $bios.component.first<GPUComponent>('gpu');
    const screen = await $bios.component.first<any>('screen');
    if (gpu && screen) {
      await gpu.bind(screen.uuid);
      // await gpu.setBackground(0x0000FF, false);
    }
    return biosCall<void>({
      name: 'bios.crash',
      args: [msg]
    });
  }

  compile(filename: string, script: string): Promise<CompileResult> {
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
}

let biosImpl = new BiosApiImpl();
global.$bios = biosImpl;

global.__bios_main = async function() {
  delete global.__bios_main;
  let eeprom = await $bios.component.first<EEPROMComponentAPI>('eeprom');
  if (!eeprom) {
    await $bios.crash('No eeprom!');
  } else {
    // Load eeprom main
    let eepromSrc = await eeprom.get();
    const result = await $bios.compile('eeprom', `(function(global, exports){
      ${eepromSrc}
    })(global, {})`);
    await $bios.log(`EEPROM compile result: ${JSON.stringify(result)}`)
    await global.__eeprom__();
    delete global.__eeprom__;
  }
}
