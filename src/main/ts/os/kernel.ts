(function () {
  //Load the require() function
  (function () {
    let handle = $bios.bootFS.open('/usr/kernel/require.js', 'r');
    let src = '';
    let read: string;
    while (read = $bios.bootFS.read(handle, 512)) src += read;
    $bios.bootFS.close(handle);
    return $bios.compile('require.js', src);
  })();

  //Load the OS
  let os = require('os');

  while (true) {
    let sig = $bios.computer.signal();
    $bios.computer.sleep(0.05);
  }
})();
