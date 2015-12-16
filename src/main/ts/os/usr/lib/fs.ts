///ts:ref=Bios.d.ts
/// <reference path="../../../bios/Bios.d.ts"/> ///ts:ref:generated
///ts:ref=Components.d.ts
/// <reference path="../../../components/Components.d.ts"/> ///ts:ref:generated

import * as path from './path';

class File {
  valid:boolean;

  constructor(private fs:components.FilesystemComponentAPI, private handle:number) {
  }

  close() {
      this.fs.close(this.handle);
  }

  read(count?:number):string {
    return this.fs.read(this.handle, count || Math.pow(2, 16));
  }

  write(data:string) {
    this.fs.write(this.handle, data);
  }
}

export class FileSystem {
  LIB_PATH = './:/usr/lib';
  PATH = '/usr/bin';
  PWD = '/';

  constructor(private root:components.FilesystemComponentAPI) {
  }

  open(filePath:string, mode:string='r'):File {
    if (!this.exists(filePath)) return null;
    let handle = this.root.open(filePath, mode);
    if (!handle) return null;
    return new File(this.root, handle);
  }

  exists(filePath:string):boolean {
    return this.root.exists(filePath);
  }

  findInPathString(pathString:string, toFind:string):string {
    if (path.isAbsolute(toFind)) return this.exists(toFind) ? toFind : null;

    let split = pathString.split(path.delimiter);
    for (let i = 0; i < split.length; i++) {
      let p = path.resolve(split[i], toFind);
      if (this.exists(p)) return p;
    }

    return null;
  }
}

export var fs = new FileSystem($bios.bootFS);
export var _singleton = true;