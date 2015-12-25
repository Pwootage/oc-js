///ts:ref=bios.d.ts
/// <reference path="../../../bios/bios.d.ts"/> ///ts:ref:generated
///ts:ref=kernel.d.ts
/// <reference path="../kernel/kernel.d.ts"/> ///ts:ref:generated
///ts:ref=component.d.ts
/// <reference path="../../../components/component.d.ts"/> ///ts:ref:generated

import term = require('term');
import component = require('component');

class OS {
  term:term.Term;

  constructor() {
    let gpu:component.GPUComponent = component.first('gpu');
    let screen:component.ScreenComponent = component.first('screen');
    gpu.bind(screen.uuid);
    gpu.fill(0, 0, gpu.getResolution()[0] + 1, gpu.getResolution()[1] + 1, 'X');

    this.term = new term.Term(gpu, {
      title: 'ocjs'
    });

    while (true) {
      let sig = $bios.computer.signal();
      if (sig) {
        if (sig.name == 'key_down') {
          if (sig.args[2] == 208) { //down arrow
            this.term.scroll(1);
          } else if (sig.args[2] == 200) { //up arrow
            this.term.scroll(-1);
          }
        }
      } else {
        $bios.computer.sleep(0.05);
      }
    }
  }
}

export = new OS();
