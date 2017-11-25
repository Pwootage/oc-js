import {EEPROMComponentAPI, FilesystemComponentAPI} from '../os/usr/lib/externalComponents';

global.__eeprom__ = async function() {
  var eeprom = await $bios.component.first<EEPROMComponentAPI>('eeprom');
  if (!eeprom) {
    await $bios.crash('No EEPROM');
    return;
  }
  let bootAddr = await eeprom.getData();
  let fs = await $bios.component.proxy<FilesystemComponentAPI>(bootAddr);
  if (!fs) {
    const filesystems = await $bios.component.list('filesystem');
    for (const fsComp of filesystems) {
      const fsToCheck = await $bios.component.proxy<FilesystemComponentAPI>(fsComp.uuid);
      if (fsToCheck) {
        const exists = await fsToCheck.exists('/kernel.js');
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
    await $bios.crash('No bootable medium found.');
    return;
  }
  $bios.bootFS = fs;
  let handle = await fs.open('/kernel.js', 'r');
  let kernel = '';
  let read: string;
  while (read = await fs.read(handle, 1024)) kernel += read;
  fs.close(handle);
  $bios.compile('kernel.js', kernel);
}
