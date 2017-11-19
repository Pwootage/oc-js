import {Term} from 'term';
import {component} from 'component';
import {GPUComponent, ScreenComponent} from './externalComponents';

class OS {
  term: Term;

  constructor() {
  }

  async init(): Promise<void> {
    let gpu: GPUComponent = await component.first('gpu');
    let screen: ScreenComponent = await component.first('screen');
    gpu.bind(screen.uuid);
    gpu.fill(0, 0, (await gpu.getResolution())[0] + 1, (await gpu.getResolution())[1] + 1, 'X');

    this.term = new Term(gpu, {
      title: 'ocjs'
    });

    while (true) {
      let sig = await $bios.computer.signal();
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
