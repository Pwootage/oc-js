//Manually wrapped since it is loaded by something that isn't require() since this is require()FileSystem

import {FileSystem} from "../lib/fs";

(function () {
  //Definitions
  class RequireImpl {
    constructor() {
      //Stub out a couple methods temporarily
      //Load from bootfs
      this.loadFile = path => {
        let handle = $bios.bootFS.open(path, 'r');
        let src = '';
        let read:string;
        while (read = $bios.bootFS.read(handle, 512)) src += read;
        $bios.bootFS.close(handle);
        return src;
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

      //Set us up as require
      global.require = this.apply.bind(this);

      //load FS
      this.fsImpl = this.apply('/usr/lib/fs.js').fs;
      this.loadFile = path => {
        let f = this.fsImpl.open(path);
        let src = '';
        let read:string;
        while (read = f.read(512)) src += read;
        f.close();
        return src;
      };
      this.find = path => this.fsImpl.findInPathString(this.fsImpl.LIB_PATH, path);

      //load path
      let path = this.apply('path.js');
      this.basename = p => path.basename(p);

      //Setup myself
      this.modules['require'] = this;
    }

    fsImpl:FileSystem;
    loadFile:(path:string)=>string;
    basename:(path:string)=>string;
    find:(path:string)=>string;

    modules:{ [path:string]: any } = {};

    apply(path:string):any {
      let moduleName = this.basename(path).replace(/\.(js|ts|json|es|es5|es6)/, '');

      //Cache
      if (this.modules[moduleName]) return this.modules[moduleName];

      let realPath = this.find(path);
      if (!realPath) return null;

      let src = this.loadFile(realPath);
      let oldExports = global.exports;
      global.exports = {};
      this.compileModule(moduleName, realPath, src);
      this.modules[moduleName] = global.exports;
      global.exports = oldExports;
      return this.modules[moduleName];
    }

    compileModule(moduleName:string, filePath:string, src:string):any {
      //Wrap in anonymous function, being careful to escape some thigns for safety
      //Notably, protect a couple vital global objects & pass a unique module object
      let modJson = JSON.stringify({
        name: moduleName,
        path: filePath
      });
      $bios.compile(filePath,
        `(function(global, $bios, require, module){${src}
        /**/if(module.exports) exports=module.exports;})(global, global.$bios, global.require, ${modJson});`);
    }
  }

  let impl = new RequireImpl();
})();
