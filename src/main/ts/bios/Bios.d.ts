declare var $bios: BiosAPI;

interface BiosAPI {
  component:BiosComponentApi;

  /** Compiles the script passed as a string */
  compile(script:string):any;

  /** Does not return. Crashes the machine! */
  crash(msg:string):void;
}

interface BiosComponentApi {
  list(filter?:string):{ [key:string]:string }
  invoke(address:string, name:string, ...args:any[]):any|any[]
  doc(address:string, name:string):string
  methods(address:string): { [key:string]:ComponentMethodInfo }
  type(address:string):string
  proxy(address:string):any
  first(type:string):any
}

interface ComponentMethodInfo {
  name:string;
  direct:boolean;
  limit:number;
  getter:boolean;
  setter:boolean;
  doc:string;
}

