import * as path from './path';
import {FilesystemComponentAPI} from './externalComponents';

class File {
  valid: boolean;

  constructor(private fs: FilesystemComponentAPI, private handle: number) {
  }

  close(): Promise<void> {
    return this.fs.close(this.handle);
  }

  read(count?: number): Promise<string> {
    return this.fs.read(this.handle, count || Math.pow(2, 16));
  }

  write(data: string): Promise<boolean> {
    return this.fs.write(this.handle, data);
  }
}

export class FileSystem {
  LIB_PATH = './:/usr/lib';
  PATH = '/usr/bin';
  PWD = '/';

  constructor(private root: FilesystemComponentAPI) {
  }

  async open(filePath: string, mode: string = 'r'): Promise<File | null> {
    if (!this.exists(filePath)) return null;
    let handle = await this.root.open(filePath, mode);
    if (!handle) return null;
    return new File(this.root, handle);
  }

  exists(filePath: string): Promise<boolean> {
    return this.root.exists(filePath);
  }

  async findInPathString(pathString: string, toFind: string): Promise<string | null> {
    $bios.log(`Finding in path str, ${pathString} ${toFind}`);
    let toFindWithExt = toFind.indexOf('.js') < 0 ? toFind + '.js' : toFind;
    if (path.isAbsolute(toFindWithExt)) return (await this.exists(toFindWithExt)) ? toFindWithExt : null;

    let split = pathString.split(path.delimiter);
    for (let i = 0; i < split.length; i++) {
      let p = path.resolve(split[i], toFindWithExt);
      if (this.exists(p)) return p;
    }

    return null;
  }
}

export var fs = new FileSystem($bios.bootFS);
