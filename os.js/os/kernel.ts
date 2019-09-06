//Load the require() function
(() => {
  let handle = $bios.bootFS.open('/usr/kernel/require.js', 'r');
  let src = '';
  let read: string;
  while (read = $bios.bootFS.read(handle, 1024)) src += read;
  $bios.bootFS.close(handle);
  $bios.compile('require.js', `(function(exports, global, define) {${src}
  /**/})({},global,function(deps,fn){return fn(null,{})})`)();
  // Load OS
  require('os');
})();
