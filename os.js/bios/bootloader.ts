import {EEPROMComponentAPI, FilesystemComponentAPI} from '../os/usr/lib/externalComponents';

(() => {
  var eeprom = $bios.component.first<EEPROMComponentAPI>('eeprom');
  if (!eeprom) {
    $bios.crash('No EEPROM');
    return;
  }
  let bootAddr = new TextDecoder('utf-8').decode(
    eeprom.getData()
  );
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
    eeprom.setData(
      new TextEncoder('utf-8').encode(fs.uuid)
    );
  }
  if (!fs) {
    $bios.crash('No bootable medium found.');
    return;
  }
  $bios.bootFS = fs;
  let kernel = $bios.readFileToString(fs, '/kernel.js');
  $bios.compile('kernel.js', kernel)();
  $bios.crash('Kernel ended execution');
})();
