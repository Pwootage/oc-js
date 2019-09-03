import {EEPROMComponentAPI, FilesystemComponentAPI, GPUComponent} from '../os/usr/lib/externalComponents';
import {
  CompileResult,
  BiosYieldResult,
  BiosYield, __yieldFunction, __compileFunction
} from './biosDefinitions';

//Private interfaces
declare var __yield: __yieldFunction;
declare var __compile: __compileFunction;

function biosYield<T>(call: BiosYield): T {
  const biosResJson = __yield(JSON.stringify(call));
  const result: BiosYieldResult = JSON.parse(biosResJson);
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

  compile(filename: string, script: string): CompileResult {
    // $bios.log(`Compiling ${filename}/${JSON.stringify(script)}`);
    return __compile(filename, script);
  }

  log(message: string) {
    biosYield({
      type: 'call',
      name: 'bios.log',
      args: [message]
    });
  }
}

let biosImpl = new BiosApiImpl();
global.$bios = biosImpl;

let eeprom = $bios.component.first<EEPROMComponentAPI>('eeprom');
if (!eeprom) {
  $bios.crash('No eeprom!');
} else {
  // Load eeprom main
  let eepromSrc = eeprom.get();
  let actaullyCompiledSrc = '';
  try {
    actaullyCompiledSrc = `(function(global, exports, define){${eepromSrc}
    })(global, {});`;

    // const huger = [];
    // let i = 0;
    // try {
    //   for (;i<1024; i++) {
    //     const arr = new Uint32Array(1024*1024 / 4);
    //     arr.fill(i);
    //     huger.push(arr);
    //     $bios.log('got to ' + i + ' ' + arr[i]);
    //   }
    // } catch (e) {
    //   conso('got to ' + i);
    //   throw e;
    // }
    // for (i = 0; i < 100; i++) {
    //   $bios.computer.sleep(1);
    // }

    $bios.compile('eeprom', actaullyCompiledSrc);
    throw new Error('EEPROM ended execution');
  } catch (error) {
    if (error) {
      const line = error.lineNumber - 1;
      const col = error.columnNumber - 1;
      const lines = actaullyCompiledSrc.split('\n');

      let context = '';
      if (line - 1 >= 0) {
        context += lines[line - 1] + '\n';
      }
      context += lines[line] + '\n';
      context += ' '.repeat(col - 1) + '^' + '\n';
      if (line + 1 < lines.length) {
        context += lines[line + 1];
      }

      $bios.crash(`Failed to compile eeprom: ${error.filename}:${error.lineNumber}:${error.columnNumber} ${error.name}/${error.message}:\n${context}\n${error}`);
    } else {
      $bios.crash(`Failed to compile eeprom: unknown error`);
    }
  }
}
