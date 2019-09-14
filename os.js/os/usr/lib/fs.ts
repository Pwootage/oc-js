import * as path from './path';
import {FilesystemComponentAPI} from 'externalComponents';

class File {
  constructor(private fs: FilesystemComponentAPI, private handle: number) {
  }

  close(): void {
    return this.fs.close(this.handle);
  }

  read(count?: number): Uint8Array {
    return this.fs.read(this.handle, count || Math.pow(2, 16));
  }

  readFullyAsString(): string {
    return $bios.readHandleToString(this.fs, this.handle);
  }

  write(data: Uint8Array): boolean {
    return this.fs.write(this.handle, data);
  }
}

export class FileSystem {
  LIB_PATH = './:/usr/lib';
  PATH = '/usr/bin';
  PWD = '/';

  constructor(private root: FilesystemComponentAPI) {
  }

  size(filePath: string): number {
    return this.root.size(filePath);
  }

  open(filePath: string, mode: string = 'r'): File | null {
    if (!this.exists(filePath)) return null;
    let handle = this.root.open(filePath, mode);
    if (!handle) return null;
    return new File(this.root, handle);
  }

  exists(filePath: string): boolean {
    return this.root.exists(filePath);
  }

  findInPathString(pathString: string, toFind: string): string | null {
    let toFindWithExt = toFind.indexOf('.js') < 0 ? toFind + '.js' : toFind;
    if (path.isAbsolute(toFindWithExt)) return this.exists(toFindWithExt) ? toFindWithExt : null;

    let split = pathString.split(path.delimiter);
    for (let i = 0; i < split.length; i++) {
      let p = path.resolve(split[i], toFindWithExt);
      if (this.exists(p)) return p;
    }

    return null;
  }
}

export var fs = new FileSystem($bios.bootFS);
