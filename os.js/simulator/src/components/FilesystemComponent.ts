import {OCComponent} from "../OCComponent";
import * as fs from 'fs-extra';
import * as path from 'path';

interface FileHandle {
  path: string;
  id: number;
  pos: number;
}

export class FilesystemComponent extends OCComponent {
  handles = new Map<number, FileHandle>();

  constructor(public rootPath: string,
              public label: string = 'root',
              public size: number = 4 * 1024 * 1024) {
    super('filesystem');

    this.registerMethod('spaceUsed', () => {
      //TODO
      return 0;
    });
    this.registerMethod('open', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      const handle = {
        path: p,
        id: fs.openSync(p, args[1]),
        pos: 0
      };
      this.handles.set(handle.id, handle);
      return handle.id;
    });
    this.registerMethod('seek', (args) => {
      const handle = this.handles.get(args[0]);
      if (!handle) {
        throw new Error('Bad handle');
      }
      const whence = args[1];
      const offset = Number(args[2]);
      if (whence == 'cur') {
        handle.pos += offset;
      } else if (whence == 'set') {
        handle.pos = offset;
      } else if (whence == 'end') {
        const len = fs.fstatSync(handle.id).size;
        handle.pos = len + offset;
      } else {
        throw new Error('invalid mode');
      }
      return handle.pos;
    });
    this.registerMethod('makeDirectory', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      fs.mkdirpSync(p);
      return true;
    });
    this.registerMethod('exists', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      return fs.existsSync(p);
    });
    this.registerMethod('isReadOnly', (args) => {
      //TODO
      return false;
    });
    this.registerMethod('spaceTotal', (args) => {
      return this.size;
    });
    this.registerMethod('isDirectory', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      return fs.statSync(p).isDirectory();
    });
    this.registerMethod('rename', (args) => {
      let p1 = this.resolvePath(args[0]);
      let p2 = this.resolvePath(args[1]);
      if (!p1.startsWith(rootPath) || !p2.startsWith(rootPath)) {
        return null;
      }
      fs.moveSync(p1, p2);
      return true;
    });
    this.registerMethod('list', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      let stat = fs.statSync(p);
      if (!stat.isDirectory()) {
        return null;
      }
      return fs.readdirSync(p);
    });
    this.registerMethod('lastModified', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      return fs.statSync(p).atime;
    });
    this.registerMethod('getLabel', (args) => {
      return this.label;
    });
    this.registerMethod('remove', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      return fs.removeSync(p);
    });
    this.registerMethod('close', (args) => {
      const handle = this.handles.get(args[0]);
      if (!handle) {
        return;
      }
      fs.close(handle.id);
      this.handles.delete(handle.id);
    });
    this.registerMethod('size', (args) => {
      let p = this.resolvePath(args[0]);
      if (!p.startsWith(rootPath)) {
        return null;
      }
      return fs.statSync(p).size;
    });
    this.registerMethod('read', (args) => {
      const handle = this.handles.get(args[0]);
      if (!handle) {
        return;
      }
      const count = Number(args[1]);
      const buffer = new Buffer(count);
      const read = fs.readSync(handle.id, buffer, 0, count, handle.pos);

      handle.pos += read;
      let str = buffer.toString('ascii').substr(0, read);
      return str;
    });
    this.registerMethod('getLabel', (args) => {
      this.label = args[0];
    });
  }

  private resolvePath(p: string): string {
    return path.resolve(this.rootPath, './' + p);
  }
}