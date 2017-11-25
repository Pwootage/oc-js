//Manually wrapped since it is loaded by something that isn't require() since this is require()

import {FileSystem} from "../lib/fs";

(function () {
  //Definitions
  class RequireImpl {
    constructor() {
    }

    private initialized = false;
    async initialize() {
      if (this.initialized == true) {
        return;
      }
      this.initialized = true;
      this.loadFile = async path => {
        let handle = await $bios.bootFS.open(path, 'r');
        let src = '';
        let read: string;
        while (read = await $bios.bootFS.read(handle, 1024)) src += read;
        $bios.bootFS.close(handle);
        return src;
      };
      //Naive but functional for what it's called with
      this.basename = path => {
        let split = path.split('/');
        return split[split.length - 1];
      };
      this.find = async path => {
        let r = path.replace(/^\.\//, '/usr/lib/');
        if (r.indexOf('.js') < 0) r = r + '.js';
        return r;
      };

      //Set us up as require & define
      global.require = this.apply.bind(this);
      this.modules['require'] = global.require;

      //load FS
      this.fsImpl = (await this.apply('/usr/lib/fs.js')).fs;
      $bios.log('FS impl: ' + JSON.stringify(this.fsImpl));

      this.loadFile = async path => {
        await $bios.log('Loading file ' + path);
        let f = await this.fsImpl.open(path);
        if (!f) {
          return null;
        }
        let src = '';
        let read: string;
        while (read = await f.read(512)) src += read;
        await f.close();
        return src;
      };
      this.find = async path => {
        $bios.log('Finding path ' + path + typeof this.fsImpl.findInPathString);
        return this.fsImpl.findInPathString(this.fsImpl.LIB_PATH, path);
      };

      //load path
      let path = await this.apply('path.js');
      this.basename = p => path.basename(p);
    }

    fsImpl: FileSystem;
    loadFile: (path: string) => Promise<string | null>;
    basename: (path: string) => string;
    find: (path: string) => Promise<string | null>;

    modules: { [path: string]: any } = {};

    async apply(path: string): Promise<any | null> {
      let moduleName = this.basename(path).replace(/\.(js|ts|json|es|es5|es6)/, '');

      $bios.log('Requiring ' + path + ':' + moduleName);
      //Cache
      if (this.modules[moduleName]) return this.modules[moduleName];

      $bios.log("Finding real path");
      let realPath = await this.find(path);
      if (!realPath) return null;

      $bios.log('Real path ' + realPath);

      let src = await this.loadFile(realPath);
      if (!src) {
        return null;
      }
      let oldExports = global.exports;
      global.exports = {};
      $bios.log('COMPILE');
      const res = await this.compileModule(moduleName, realPath, src);
      $bios.log("Compile done: " + JSON.stringify(global.exports));
      let value: any;
      value = global.exports;
      global.exports = oldExports;
      this.modules[moduleName] = value;
      $bios.log('Finished getting module: ' + JSON.stringify(value));
      return this.modules[moduleName];
    }

    compileModule(moduleName: string, filePath: string, src: string): Promise<any> {
      //Wrap in anonymous function, being careful to escape some thigns for safety
      //Notably, protect a couple vital global objects & pass a unique module object
      let modJson = JSON.stringify({
        name: moduleName,
        path: filePath
      });
      return $bios.compile(filePath,
        `(function(global, exports, $bios, require, module){${src}
        /**/if(module.exports) exports=module.exports; global.exports = exports;})(global, global.exports, global.$bios, global.require, ${modJson});`);
    }
  }

  let impl = new RequireImpl();
  global.__require_init = impl.initialize().then(() => {
    $bios.log('Require initialized');
    delete global.__require_init;
  }).catch((error) => {
    $bios.log('Require crash ' + JSON.stringify(error));
    delete global.__require_init;
    throw error;
  });
})();
