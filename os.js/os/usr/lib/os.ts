import { Term } from 'usr/lib/term';
import { component } from 'usr/lib/component';
import { GPUComponent, ScreenComponent } from './externalComponents';

class OS {
  term: Term;

  constructor() {
    let gpu: GPUComponent = component.first('gpu');
    let screen: ScreenComponent = component.first('screen');
    if (!gpu) {
      $bios.crash('Missing GPU!');
    }
    if (!screen) {
      $bios.crash('Missing screen!');
    }
    gpu.bind(screen.uuid);
    gpu.fill(0, 0, gpu.getResolution()[0] + 1, gpu.getResolution()[1] + 1, 'X');

    this.term = new Term(gpu, {
      title: 'ocjs'
    });

    $bios.signals.on('signal', (sig) => {
      if (sig.name == 'key_down') {
        if (sig.args[2] == 208) { //down arrow
          this.term.scroll(1);
        } else if (sig.args[2] == 200) { //up arrow
          this.term.scroll(-1);
        }
      }
    });

    while (true) {
      $bios.computer.sleep(1);
    }
  }
}

export const instance = new OS();
