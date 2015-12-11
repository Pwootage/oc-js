var module, require;

!function() {
    module = {
        exports: null
    }, require = function() {
        for (var a, b = $bios.bootFS.open("/usr/kernel/require.js", "r"), c = ""; a = $bios.bootFS.read(b, 512); ) c += a;
        return $bios.bootFS.close(b), $bios.compile("requre.js", c);
    }();
    var a = $bios.component.first("gpu"), b = $bios.component.first("screen");
    a.bind(b.uuid), a.setBackground(0), a.setForeground(16777215);
    var c = a.getResolution();
    for (a.fill(1, 1, c[0] + 1, c[1] + 1, " "), a.set(1, 1, "Hello, Minecraft!"); ;) {
        var d = $bios.computer.signal();
        d && (a.copy(1, 1, c[0] + 1, c[1], 0, 1), a.fill(1, 1, c[0] + 1, 1, " "), a.set(1, 1, d.name + ":" + d.args)), 
        $bios.computer.sleep(.05);
    }
}();