import {FilesystemComponentAPI} from './externalComponents';

declare global {
  var $bios: BiosApi;
  var global: any;

  class BiosApi {
    component: BiosComponentApi;
    computer: BiosComputerApi;
    bootFS: FilesystemComponentAPI;

    /** Compiles the script passed as a string */
    compile(filename: string, script: string): any;
    /** Does not return. Crashes the machine! */
    crash(msg: string): void;
    log(msg: string): void;
  }

  interface BiosComponentApi {
    list(filter?: string): Promise<ComponentInfo[]>;
    invoke(address: string, name: string, ...args: any[]): Promise<any | any[]>;
    doc(address: string, name: string): Promise<string>;
    methods(address: string): Promise<{
      [key: string]: ComponentMethodInfo;
    }>;
    type(address: string): Promise<string>;
    proxy<T>(address: string): Promise<T | null>;
    first<T>(type: string): Promise<T | null>;
  }

  interface BiosComputerApi {
    signal(): Promise<Signal | null>;
    sleep(time: number): Promise<void>;
    address(): Promise<string>;
    tmpAddress(): Promise<string>;
    freeMemory(): Promise<number>;
    totalMemory(): Promise<number>;
    energy(): Promise<number>;
    maxEnergy(): Promise<number>;
    uptime(): Promise<number>;
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
