var $bios, __bios__ = function(a) {
    __bios__ = null;
    var b = function() {
        function b() {}
        return b.prototype.list = function(b) {
            return $bios.javaArrayToList(a.component.list(b || ""));
        }, b.prototype.invoke = function(b, c) {
            for (var d = [], e = 2; e < arguments.length; e++) d[e - 2] = arguments[e];
            var f = a.component.invoke(b, c, d);
            return f && 1 === f.length ? f[0] : f;
        }, b.prototype.doc = function(b, c) {
            return a.component.doc(b, c);
        }, b.prototype.methods = function(b) {
            return a.component.methods(b);
        }, b.prototype.type = function(b) {
            return a.component.type(b);
        }, b.prototype.proxy = function(a) {
            var b = this, c = {}, d = this.methods(a);
            if (!d) return null;
            Object.defineProperty(c, "uuid", {
                enumerable: !0,
                value: a
            });
            for (var e in d) {
                var f = d[e], g = function(c) {
                    return function() {
                        return b.invoke.apply(b, [ a, c ]);
                    };
                }, h = function(c, d) {
                    return b.invoke.apply(b, [ a, c, d ]);
                };
                if (f.getter || f.setter) Object.defineProperty(c, f.name, {
                    enumerable: !0,
                    get: f.getter ? g.bind(this, f.name) : null,
                    set: f.setter ? h.bind(this, f.name) : null
                }); else {
                    var i = function(c) {
                        for (var d = [], e = 1; e < arguments.length; e++) d[e - 1] = arguments[e];
                        return b.invoke.apply(b, [ a, c ].concat(d));
                    };
                    Object.defineProperty(c, f.name, {
                        enumerable: !0,
                        writable: !1,
                        value: i.bind(this, f.name)
                    });
                }
            }
            return c;
        }, b.prototype.first = function(a) {
            var b = $bios.component.list(a)[0].uuid;
            return b ? $bios.component.proxy(b) : null;
        }, b;
    }(), c = function() {
        function b() {}
        return b.prototype.signal = function() {
            var b = a.computer.signal();
            return b ? {
                name: b.name,
                args: $bios.javaArrayToList(b.args)
            } : null;
        }, b.prototype.sleep = function(b) {
            a.computer.sleep(b);
        }, b.prototype.address = function() {
            return a.computer.address();
        }, b.prototype.tmpAddress = function() {
            return a.computer.tmpAddress();
        }, b.prototype.freeMemory = function() {
            return a.computer.freeMemory();
        }, b.prototype.totalMemory = function() {
            return a.computer.totalMemory();
        }, b.prototype.energy = function() {
            return a.computer.energy();
        }, b.prototype.maxEnergy = function() {
            return a.computer.maxEnergy();
        }, b.prototype.uptime = function() {
            return a.computer.uptime();
        }, b;
    }(), d = function() {
        function d() {
            this.component = new b(), this.computer = new c();
        }
        return d.prototype.crash = function(b) {
            var c = $bios.component.first("gpu"), d = $bios.component.first("screen");
            c && d && c.bind(d.uuid), a.bios.crash(b);
        }, d.prototype.compile = function(b, c) {
            return a.bios.compile(b, c);
        }, d.prototype.javaArrayToList = function(a) {
            for (var b = [], c = 0; c < a.length; c++) b.push(a[c]);
            return b;
        }, d;
    }();
    $bios = new d();
    var e = $bios.component.first("eeprom");
    e ? $bios.compile("eeprom", e.get()) : $bios.crash("No eeprom!");
};