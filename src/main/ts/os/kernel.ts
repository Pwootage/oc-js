(() => {
  //Load the require() function
  (async () => {
    let handle = await $bios.bootFS.open('/usr/kernel/require.js', 'r');
    let src = '';
    let read: string;
    while (read = await $bios.bootFS.read(handle, 512)) src += read;
    await $bios.bootFS.close(handle);
    // return $bios.compile('require.js', src);
  })().then(async () => {
    //Load the OS
    // let os = require('os');

    while (true) {
      let sig = await $bios.computer.signal();
      await $bios.computer.sleep(0.05);
    }
  });
})();
