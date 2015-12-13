///ts:ref=Bios.d.ts
/// <reference path="../../../bios/Bios.d.ts"/> ///ts:ref:generated
///ts:ref=Components.d.ts
/// <reference path="../../../components/Components.d.ts"/> ///ts:ref:generated

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
  open(path:string):File {
    return new File($bios.bootFS, 0);
  }
}

export var fs = new FileSystem();
export var _singleton = true;