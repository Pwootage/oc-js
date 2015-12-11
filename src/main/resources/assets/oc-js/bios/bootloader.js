!function() {
    var a = $bios.component.first("eeprom"), b = a.getData(), c = $bios.component.proxy(b);
    c || (c = $bios.component.list("filesystem").map(function(a) {
        return $bios.component.proxy(a.uuid);
    }).filter(function(a) {
        return a.exists("/kernel.js");
    })[0], c && a.setData(c.uuid)), c || $bios.crash("No bootable medium found."), $bios.bootFS = c;
    for (var d, e = c.open("kernel.js", "r"), f = ""; d = c.read(e, 512); ) f += d;
    c.close(e), $bios.compile("kernel.js", f);
}();