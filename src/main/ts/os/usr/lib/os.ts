import {Term} from "term";
import {component, GPUComponent, ScreenComponent} from "component";

class OS {
  term: Term;

  constructor() {
    let gpu: GPUComponent = component.first('gpu');
    let screen: ScreenComponent = component.first('screen');
    gpu.bind(screen.uuid);
    gpu.fill(0, 0, gpu.getResolution()[0] + 1, gpu.getResolution()[1] + 1, 'X');

    this.term = new Term(gpu, {
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
