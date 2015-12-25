///ts:ref=component.d.ts
/// <reference path="../components/component.d.ts"/> ///ts:ref:generated

declare var $bios:bios.BiosApi;
declare var global:any;

declare module bios {
  import FilesystemComponentAPI = component.FilesystemComponentAPI;
  export interface BiosApi {
    component:BiosComponentApi;
    computer:BiosComputerApi;
    bootFS:FilesystemComponentAPI;

    /** Compiles the script passed as a string */
    compile(filename:string, script:string):any;

    /** Does not return. Crashes the machine! */
    crash(msg:string):void;

    log(msg:string);

    javaArrayToList<T>(arr:T[]): T[];
  }

  export interface BiosComponentApi {
    list(filter?:string):ComponentInfo[]
    invoke(address:string, name:string, ...args:any[]):any|any[]
    doc(address:string, name:string):string
    methods(address:string): { [key:string]:ComponentMethodInfo }
    type(address:string):string
    proxy(address:string):any
    first(type:string):any
  }

  export interface BiosComputerApi {
    signal():Signal
    sleep(time:number):void
    address():string
    tmpAddress():string
    freeMemory():number
    totalMemory():number
    energy():number
    maxEnergy():number
    uptime():number
  }

  export interface ComponentInfo {
    uuid:string;
    type:string;
  }

  export interface ComponentMethodInfo {
    name:string;
    direct:boolean;
    limit:number;
    getter:boolean;
    setter:boolean;
    doc:string;
  }

  export interface Signal {
    name:string;
    args:any[];
  }
}
