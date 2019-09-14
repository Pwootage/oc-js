//Load the require() function
(() => {
  const src = $bios.readFileToString($bios.bootFS, '/usr/kernel/require.js');
  $bios.compile('require.js', `(function(exports, global, define) {${src}
  /**/})({},global,function(deps,fn){return fn(null,{})})`)();
  // Load OS
  require('os');
})();
