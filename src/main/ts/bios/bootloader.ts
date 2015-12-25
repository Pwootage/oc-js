///ts:ref=Components.d.ts
/// No file or directory matched name "Components.d.ts" ///ts:ref:generated
///ts:ref=bios.d.ts
/// <reference path="./bios.d.ts"/> ///ts:ref:generated

import EEPROMComponentAPI = component.EEPROMComponentAPI;
import FilesystemComponentAPI = component.FilesystemComponentAPI;

(function () {
  var eeprom:EEPROMComponentAPI = $bios.component.first('eeprom');
  let bootAddr = eeprom.getData();
  let fs:FilesystemComponentAPI = $bios.component.proxy(bootAddr);
  if (!fs) {
    fs = $bios.component.list('filesystem')
      .map(v => <FilesystemComponentAPI>$bios.component.proxy(v.uuid))
      .filter(v => v.exists('/kernel.js'))[0];
    if (fs) eeprom.setData(fs.uuid);
  }
  if (!fs) {
    $bios.crash('No bootable medium found.');
  }
  $bios.bootFS = fs;
  let handle = fs.open('kernel.js', 'r');
  let kernel = '';
  let read:string;
  while (read = fs.read(handle, 512)) kernel += read;
  fs.close(handle);
  $bios.compile('kernel.js', kernel);
})();
