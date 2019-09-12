//Load the require() function
(() => {
  const handle = $bios.bootFS.open('/usr/kernel/require.js', 'r');
  const size = $bios.bootFS.size('/usr/kernel/require.js');
  const src =  new TextDecoder('utf-8').decode(
    $bios.bootFS.read(handle, size)
  );
  $bios.bootFS.close(handle);
  $bios.compile('require.js', `(function(exports, global, define) {${src}
  /**/})({},global,function(deps,fn){return fn(null,{})})`)();
  // Load OS
  require('os');
})();
