import {FilesystemComponentAPI} from './externalComponents';

declare global {
  var $bios: BiosApi;
  var global: any;

  interface BiosApi {
    component: BiosComponentApi;
    computer: BiosComputerApi;
    bootFS: FilesystemComponentAPI;
    /** Compiles the script passed as a string */
    compile(filename: string, script: string): any;
    /** Does not return. Crashes the machine! */
    crash(msg: string): void;
    log(msg: string): void;
    javaArrayToList<T>(arr: T[]): T[];
  }

  interface BiosComponentApi {
    list(filter?: string): ComponentInfo[];
    invoke(address: string, name: string, ...args: any[]): any | any[];
    doc(address: string, name: string): string;
    methods(address: string): {
      [key: string]: ComponentMethodInfo;
    };
    type(address: string): string;
    proxy(address: string): any;
    first(type: string): any;
  }

  interface BiosComputerApi {
    signal(): Signal;
    sleep(time: number): void;
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
