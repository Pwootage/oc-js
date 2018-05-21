"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
(() => {
    var eeprom = $bios.component.first('eeprom');
    if (!eeprom) {
        $bios.crash('No EEPROM');
        return;
    }
    let bootAddr = eeprom.getData();
    let fs = $bios.component.proxy(bootAddr);
    if (!fs) {
        const filesystems = $bios.component.list('filesystem');
        for (const fsComp of filesystems) {
            const fsToCheck = $bios.component.proxy(fsComp.uuid);
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
    let read;
    while (read = fs.read(handle, 1024))
        kernel += read;
    fs.close(handle);
    $bios.compile('kernel.js', kernel);
    $bios.crash('Kernel ended execution');
})();
