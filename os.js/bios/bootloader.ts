import {EEPROMComponentAPI, FilesystemComponentAPI} from '../os/usr/lib/externalComponents';

(() => {
  var eeprom = $bios.component.first<EEPROMComponentAPI>('eeprom');
  if (!eeprom) {
    $bios.crash('No EEPROM');
    return;
  }
  let bootAddr = eeprom.getData();
  let fs = $bios.component.proxy<FilesystemComponentAPI>(bootAddr);
  if (!fs) {
    const filesystems = $bios.component.list('filesystem');
    for (const fsComp of filesystems) {
      const fsToCheck = $bios.component.proxy<FilesystemComponentAPI>(fsComp.uuid);
      if (fsToCheck) {
        const exists = fsToCheck.exists('/kernel.js');
        if (exists) {
          fs = fsToCheck;
          break;
        }
      }
    }
  }
  if (fs) {
    eeprom.setData(fs.uuid);
  }
  if (!fs) {
    $bios.crash('No bootable medium found.');
    return;
  }
  $bios.bootFS = fs;
  let handle = fs.open('/kernel.js', 'r');
  let kernel = '';
  let read: string;
  while (read = fs.read(handle, 1024)) kernel += read;
  fs.close(handle);
  $bios.compile('kernel.js', kernel)();
  $bios.crash('Kernel ended execution');
})();
