import {EEPROMComponentAPI, FilesystemComponentAPI, GPUComponent} from '../os/usr/lib/externalComponents';
import {
  BiosYieldResult,
  BiosYield, __yieldFunction, __compileFunction
} from './biosDefinitions';

if (!global.hasOwnProperty('global')) {
  Object.defineProperty(global, 'global', {
    enumerable: false,
    value: global,
    writable: false,
    configurable: false
  });
}

//Private interfaces
declare var __yield: __yieldFunction;
declare var __compile: __compileFunction;

function biosYield<T>(call: BiosYield): T {
  const result = __yield(call);
  if (!result) {
    return result;
  } else if (result.state == 'success') {
    return result.value
  } else {
    throw new Error(`Error throw in java from call ${JSON.stringify(call)}: ${JSON.stringify(result)}`);
  }
}

class BiosComponentApiImpl implements BiosComponentApi {
  list(filter?: string | RegExp): ComponentInfo[] {
    const arr = biosYield<ComponentInfo[]>({
      type: 'call',
      name: 'component.list',
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

  invoke(address: string, name: string, ...args: any[]): any | any[] {
    const res = biosYield<any[]>({
      type: 'call',
      name: 'component.invoke',
      args: [address, name, args]
    });
    // $bios.log(`Component invoke res ${name}/${JSON.stringify(res)}`);
    if (!res || res.length == 0) {
      return undefined;
    } else if (res.length == 1) {
      return res[0];
    } else {
      return res;
    }
  }

  doc(address: string, name: string): string {
    return biosYield<string>({
      type: 'call',
      name: 'component.doc',
      args: [address, name]
    });
  }

  methods(address: string): { [key: string]: ComponentMethodInfo } {
    return biosYield<{ [key: string]: ComponentMethodInfo }>({
      type: 'call',
      name: 'component.methods',
      args: [address]
    });
  }

  type(address: string): string {
    return biosYield<string>({
      type: 'call',
      name: 'component.type',
      args: [address]
    });
  }

  proxy<T>(address: string): T | null {
    const res: any = {};
    const methods = this.methods(address);
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
          'get': method.getter ? g.bind(this, method.name) : undefined,
          'set': method.setter ? s.bind(this, method.name) : undefined
        });
      } else {
        let i = (n: string, ...args: any[]) => this.invoke.apply(this, [address, n, ...args]);
        Object.defineProperty(res, method.name, {
          enumerable: true,
          writable: false,
          value: i.bind(this, method.name)
        });
      }
    }
    return res;
  }

  first<T>(type: string): T | null {
    const list = this.list(type);
    const address = list[0] && list[0].uuid;
    if (!address) {
      return null;
    } else {
      return $bios.component.proxy<T>(address);
    }
  }
}

class BiosComputerApiImpl implements BiosComputerApi {
  sleep(duration: number): Signal | null {
    const signal: Signal | null = biosYield({
      type: 'sleep',
      duration
    });
    if (signal) {
      $bios.signals.emit('signal', signal);
      $bios.signals.emit(signal.name, signal.args);
    }
    return signal;
  }

  address(): string {
    return biosYield({
      type: 'call',
      name: 'computer.address',
      args: []
    });
  }

  tmpAddress(): string {
    return biosYield({
      type: 'call',
      name: 'computer.tmpAddress',
      args: []
    });
  }

  freeMemory(): number {
    return biosYield({
      type: 'call',
      name: 'computer.freeMemory',
      args: []
    });
  }

  totalMemory(): number {
    return biosYield({
      type: 'call',
      name: 'computer.totalMemory',
      args: []
    });
  }

  energy(): number {
    return biosYield({
      type: 'call',
      name: 'computer.energy',
      args: []
    });
  }

  maxEnergy(): number {
    return biosYield({
      type: 'call',
      name: 'computer.maxEnergy',
      args: []
    });
  }

  uptime(): number {
    return biosYield({
      type: 'call',
      name: 'computer.uptime',
      args: []
    });
  }

}

type DeregFunction = () => void;
type EventHandler = (...args: any[]) => void;

class EventEmitter {
  private listeners = new Map<string, EventHandler[]>();

  constructor() {
  }

  on(name: string, fn: EventHandler): DeregFunction {
    let arr = this.listeners.get(name) || [];
    arr.push(fn);
    this.listeners.set(name, arr);
    return this.deregister.bind(this, name, fn);
  }

  emit(name: string, ...args: any[]): void {
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
  /** Set by the bootloader */
  bootFS!: FilesystemComponentAPI;
  signals = new EventEmitter();

  crash(msg: string): void {
    const gpu = $bios.component.first<GPUComponent>('gpu');
    const screen = $bios.component.first<any>('screen');
    if (gpu && screen) {
      gpu.bind(screen.uuid);
      gpu.setBackground(0x0000FF, false);
    }
    return biosYield<void>({
      type: 'call',
      name: 'bios.crash',
      args: [msg]
    });
  }

  compile(filename: string, script: string): () => void {
    // $bios.log(`Compiling ${filename}/${JSON.stringify(script)}`);
    try {
      return __compile(filename, script);
    } catch (error) {
      if (error) {
        let line = 0, col = 0;

        if (error.lineNumber) {
          line = error.lineNumber - 1;
        }
        if (error.columnNumber) {
          col = error.columnNumber - 1;
        }

        const lines = script.split('\n');
        $bios.log(`Lines: ${lines.length}, line: ${line}`);

        let context = '';
        if (line - 3 >= 0) {
          context += lines[line - 3] + '\n';
        }
        if (line - 2 >= 0) {
          context += lines[line - 2] + '\n';
        }
        if (line - 1 >= 0) {
          context += lines[line - 1] + '\n';
        }
        context += lines[line] + '\n';
        context += ' '.repeat(Math.max(col - 1, 0)) + '^' + '\n';
        if (line + 1 < lines.length) {
          context += lines[line + 1];
        }

        error.prettyMessage = `${error.filename}:${error.lineNumber}:${error.columnNumber} ${error.name}/${error.message}:\n${context}\n${error}`;
      }
      throw error;
    }
  }

  log(message: string) {
    biosYield({
      type: 'call',
      name: 'bios.log',
      args: [message]
    });
  }

  readFileToString(fs: FilesystemComponentAPI, path: string): string {
    const handle = fs.open(path, 'r');
    const res = this.readHandleToString(fs, handle);
    fs.close(handle);
    return res;
  }

  readHandleToString(fs: FilesystemComponentAPI, handle: number): string {
    let buffers = [];
    let read;
    while ((read = fs.read(handle, 2048))) {
      if (read.length == 0) break;
      buffers.push(read);
    }
    let size = 0;
    for (let i = 0; i < buffers.length; i++) {
      size += buffers[i].length;
    }
    const buffer = new Uint8Array(size);
    let offset = 0;
    for (let i = 0; i < buffers.length; i++) {
      buffer.set(buffers[i], offset);
      offset += buffers[i].length;
    }
    return new TextDecoder('utf-8').decode(buffer);
  }
}

let biosImpl = new BiosApiImpl();
global.$bios = biosImpl;

let eeprom = $bios.component.first<EEPROMComponentAPI>('eeprom');
if (!eeprom) {
  $bios.crash('No eeprom!');
} else {
  // Load eeprom main
  let eepromSrc = new TextDecoder('utf-8').decode(
    eeprom.get()
  );
  let actaullyCompiledSrc = '';
  let ee;
  try {
    actaullyCompiledSrc = `(function(global, exports, define){${eepromSrc}
    })(global, {});`;

    ee = $bios.compile('eeprom', actaullyCompiledSrc);
  } catch (error) {
    $bios.crash(`Failed to compile eeprom: ${error.prettyMessage || error.message} ${error.stack}`);
  }
  try {
    if (ee) ee();
    throw new Error('EEPROM ended execution');
  } catch (error) {
    $bios.crash(`Failed to run eeprom: ${error.prettyMessage || error.message} ${error.stack}`);
  }
}
