var File = function() {
    function a(a) {}
    return a;
}(), FileSystem = function() {
    function a() {}
    return a.prototype.open = function(a) {
        return new File(0);
    }, a;
}();

exports.FileSystem = FileSystem, exports.fs = new FileSystem();