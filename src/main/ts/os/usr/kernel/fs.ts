///ts:ref=Bios.d.ts
/// <reference path="../../../bios/Bios.d.ts"/> ///ts:ref:generated
///ts:ref=Components.d.ts
/// <reference path="../../../components/Components.d.ts"/> ///ts:ref:generated

class File {
  constructor(handle:number) {

  }

  handle:number;
}

export class FileSystem {
  open(path:string):File {
    return new File(0);
  }
}

export var fs = new FileSystem();