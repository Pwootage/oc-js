(function() {
  let gpu:GPUComponent = $bios.component.first('gpu');
  let screen:ScreenComponent = $bios.component.first('screen');
  gpu.bind(screen.uuid);
  gpu.setBackground(0x000000);
  gpu.setForeground(0xFFFFFF);
  let size = gpu.getResolution();
  gpu.fill(1, 1, size[0] + 1, size[1] + 1, ' ');

  while (true) {
    let sig = $bios.computer.signal();
    if (sig) {
      gpu.copy(1, 1, size[0] + 1, size[1], 0, 1);
      gpu.fill(1, 1, size[0] + 1, 1, ' ');
      gpu.set(1, 1, sig.name + ':' + sig.args);
    }
    $bios.computer.sleep(0.05);
  }
})();