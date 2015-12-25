///ts:ref=Bios.d.ts
/// No file or directory matched name "Bios.d.ts" ///ts:ref:generated
///ts:ref=component.d.ts
/// <reference path="../components/component.d.ts"/> ///ts:ref:generated
///ts:ref=kernel.d.ts
/// <reference path="./usr/kernel/kernel.d.ts"/> ///ts:ref:generated

import ScreenComponent = component.ScreenComponent;
import GPUComponent = component.GPUComponent;

(function () {
  //Load the require() function
  (function () {
    let handle = $bios.bootFS.open('/usr/kernel/require.js', 'r');
    let src = '';
    let read:string;
    while (read = $bios.bootFS.read(handle, 512)) src += read;
    $bios.bootFS.close(handle);
    return $bios.compile('require.js', src);
  })();

  //Load the OS
  let os = require('os');

  while (true) {
    let sig = $bios.computer.signal();
    $bios.computer.sleep(0.05);
  }
})();
