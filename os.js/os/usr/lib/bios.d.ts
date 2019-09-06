import {FilesystemComponentAPI} from './externalComponents';

declare global {
  var $bios: BiosApi;
  var global: any;

  type DeregFunction = () => void;
  type EventHandler = (...args: any[]) => void;

  class EventEmitter {
    constructor();
    on(name:string, fn: EventHandler): DeregFunction;
    emit(name:string, ...args: any[]): void;
    deregister(name: string, fn: EventHandler): void;
  }

  class BiosApi {
    component: BiosComponentApi;
    computer: BiosComputerApi;
    bootFS: FilesystemComponentAPI;

    signals: EventEmitter

    /** Compiles the script passed as a string */
    compile(filename: string, script: string): () => void;
    /** Does not return. Crashes the machine! */
    crash(msg: string): void;
    log(msg: string): void;
  }

  interface BiosComponentApi {
    list(filter?: string): ComponentInfo[];
    invoke(address: string, name: string, ...args: any[]): any | any[];
    doc(address: string, name: string): string;
    methods(address: string): {
      [key: string]: ComponentMethodInfo;
    };
    type(address: string): string;
    proxy<T>(address: string): T | null;
    first<T>(type: string): T | null;
  }

  interface BiosComputerApi {
    sleep(duration: number): void;
    address(): string;
    tmpAddress(): string;
    freeMemory(): number;
    totalMemory(): number;
    energy(): number;
    maxEnergy(): number;
    uptime(): number;
  }

  interface ComponentInfo {
    uuid: string;
    type: string;
  }

  interface ComponentMethodInfo {
    name: string;
    direct: boolean;
    limit: number;
    getter: boolean;
    setter: boolean;
    doc: string;
  }

  interface Signal {
    name: string;
    args: any[];
  }
}
