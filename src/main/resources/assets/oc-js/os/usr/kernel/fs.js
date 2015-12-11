var File = function() {
    function a(a, b) {
        this.fs = a, this.handle = b;
    }
    return a.prototype.close = function() {
        this.fs.close(this.handle);
    }, a.prototype.read = function(a) {
        this.fs.read(this.handle, a || Math.pow(2, 16));
    }, a;
}(), FileSystem = function() {
    function a() {}
    return a.prototype.open = function(a) {
        return new File($bios.bootFS, 0);
    }, a;
}();

exports.FileSystem = FileSystem, exports.fs = new FileSystem(), exports._singleton = !0;