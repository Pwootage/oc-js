declare var require:(file:string)=>any;

declare var exports:any;
declare var module:ModuleInfo;

interface ModuleInfo {
  name:string;
  path:string;
}
