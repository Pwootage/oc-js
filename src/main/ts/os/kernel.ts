//Load the require() function
(async () => {
  let handle = await $bios.bootFS.open('/usr/kernel/require.js', 'r');
  let src = '';
  let read: string;
  while (read = await $bios.bootFS.read(handle, 1024)) src += read;
  await $bios.bootFS.close(handle);
  await $bios.compile('require.js', `(function(exports, global, define) {${src}
/**/})({}, global, (deps, fn) => fn(null, {}))`);
  await global.__require_init;
  let os = await require('os');
  os.init();
})().then(async () => {
}).catch((error) => {
  error = error || new Error('Unknown error');
  let stack = error.stack;
  if (!stack) {
    if (error.toString) {
      stack = error.toString();
    } else {
      stack = JSON.stringify(error);
    }
  }
  $bios.crash(`Kernel panic: ${stack}`);
});
