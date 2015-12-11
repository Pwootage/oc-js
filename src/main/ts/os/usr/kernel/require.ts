///ts:ref=Bios.d.ts
/// <reference path="../../../bios/Bios.d.ts"/> ///ts:ref:generated
///ts:ref=kernel.d.ts
/// <reference path="./kernel.d.ts"/> ///ts:ref:generated
///ts:ref=fs.ts
/// <reference path="./fs.ts"/> ///ts:ref:generated

//Manually wrapped since it is loaded by something that isn't require()

(function () {
  let fs = require("./fs");
  function require(file:string) {

  }

  function loadFileFromBiosFS(path:string) {
    let handle = $bios.bootFS.open(path, 'r');
    let src = '';
    let read:string;
    while (read = $bios.bootFS.read(handle, 512)) src += read;
    $bios.bootFS.close(handle);
    return src;
  }

  function loadFileFromFS(path:string) {

  }

  function compileSource(src:string) {

  }

  exports = require;
})();
