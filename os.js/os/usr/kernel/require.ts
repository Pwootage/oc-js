//Manually wrapped since it is loaded by something that isn't require() since this is require()

import {FileSystem} from "../lib/fs";

(function () {
  //Definitions
  class RequireImpl {
    constructor() {
      this.loadFile = path => {
        return $bios.readFileToString($bios.bootFS, path);
      };
      //Naive but functional for what it's called with
      this.basename = path => {
        let split = path.split('/');
        return split[split.length - 1];
      };
      this.find = path => {
        let r = path.replace(/^\.\//, '/usr/lib/');
        if (r.indexOf('.js') < 0) r = r + '.js';
        return r;
      };

      //Set us up as require & define
      global.require = this.apply.bind(this);
      this.modules['require'] = global.require;

      //load FS
      this.fsImpl = this.apply('/usr/lib/fs.js').fs;

      this.loadFile = path => {
        const f = this.fsImpl.open(path);
        if (!f) return null;
        let src = f.readFullyAsString();
        f.close();
        return src;
      };
      this.find = path => {
        return this.fsImpl.findInPathString(this.fsImpl.LIB_PATH, path);
      };

      //load path
      let path = this.apply('path.js');
      this.basename = p => path.basename(p);
    }

    fsImpl: FileSystem;
    loadFile: (path: string) => string | null;
    basename: (path: string) => string;
    find: (path: string) => string | null;

    modules: { [path: string]: any } = {};

    apply(path: string): any | null {
      let moduleName = this.basename(path).replace(/\.(js|ts|json|es|es5|es6)/, '');

      //Cache
      if (this.modules[moduleName]) return this.modules[moduleName];

      let realPath = this.find(path);
      if (!realPath) return null;

      let src = this.loadFile(realPath);
      if (!src) {
        return null;
      }
      let oldExports = global.exports;
      global.exports = {};
      const res = this.compileModule(moduleName, realPath, src);
      let value: any;
      value = global.exports;
      global.exports = oldExports;
      this.modules[moduleName] = value;
      return this.modules[moduleName];
    }

    compileModule(moduleName: string, filePath: string, src: string): any {
      //Wrap in anonymous function, being careful to escape some thigns for safety
      //Notably, protect a couple vital global objects & pass a unique module object
      let modJson = JSON.stringify({
        name: moduleName,
        path: filePath
      });
      return $bios.compile(filePath,
        `(function(global, exports, $bios, require, module){${src}
        /**/if(module.exports) exports=module.exports; global.exports = exports;})(global, global.exports, global.$bios, global.require, ${modJson});`)();
    }
  }
  global.__requireImpl = new RequireImpl();
})();
