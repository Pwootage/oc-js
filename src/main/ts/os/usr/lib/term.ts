/** Contains config options for a terminal */
import {GPUComponent} from 'externalComponents';

export interface TermConfig {
  /** X starting position on screen (default: 1) */
  startX?: number;
  /** Y starting position on screen (default: 1) */
  startY?: number;
  /** X ending position on screen (default: width of screen) */
  endX?: number;
  /** Y ending position on screen (default: height of screen )*/
  endY?: number;
  /** Term title (default: '') */
  title?: string;
  /** Lines of scrollback stored (default: 500) */
  scrollback?: number;
}

//SEE: https://en.wikipedia.org/wiki/Box-drawing_character
//   │ ┤ ╡ ╢ ╖ ╕ ╣ ║ ╗ ╝ ╜ ╛ ┐
//└ ┴ ┬ ├ ─ ┼ ╞ ╟ ╚ ╔ ╩ ╦ ╠ ═ ╬ ╧
//╨ ╤ ╥ ╙ ╘ ╒ ╓ ╫ ╪ ┘ ┌

/** Represents a terminal. */
export class Term {
  private startX: number;
  private startY: number;
  private endX: number;
  private endY: number;
  private width: number;
  private height: number;
  private scrollbackMax: number;
  private buffer: string[];
  private currentBottom: number;
  private lastBottom: number;

  private title: string;

  constructor(private gpu: GPUComponent, config?: TermConfig) {
    this.init(gpu, config).then(() => {

    });
  }

  async init(gpu: GPUComponent, config?: TermConfig): Promise<void> {
    config = config || {};
    this.startX = config.startX || 1;
    this.startY = config.startY || 1;
    this.endX = config.endX || ((await gpu.getResolution())[0]);
    this.endY = config.endY || ((await gpu.getResolution())[1]);
    this.title = config.title || '';
    this.scrollbackMax = config.scrollback || 110;
    this.buffer = new Array(this.scrollbackMax);
    this.currentBottom = this.scrollbackMax - 1;
    this.lastBottom = 0;

    for (let i = 0; i < this.scrollbackMax; i++) {
      this.buffer[i] = `${i} data ${i}`;
    }

    this.width = this.endX - this.startX + 1;
    this.height = this.endY - this.startY + 1;

    this.drawBorder();
    this.clearCenter();
    this.drawTitle();
    this.updateScreen();
  }

  drawBorder() {
    this.gpu.setForeground(0xFFFFFF);
    this.gpu.setBackground(0x000000);
    //Top bar
    this.gpu.fill(this.startX + 1, this.startY, this.width - 2, 1, '═');
    //Bottom bar
    this.gpu.fill(this.startX + 1, this.endY, this.width - 2, 1, '═');

    //Left bar
    this.gpu.fill(this.startX, this.startY + 1, 1, this.height - 2, '║');
    //Right bar is scrollbar
    //Corners
    this.gpu.set(this.startX, this.startY, '╔');
    this.gpu.set(this.startX, this.endY, '╚');
    this.gpu.set(this.endX, this.startY, '╕');
    this.gpu.set(this.endX, this.endY, '╛')
  }

  drawTitle() {
    this.gpu.set(this.startX + 2, this.startY, this.title);
  }

  clearCenter() {
    this.gpu.setForeground(0xFFFFFF);
    this.gpu.setBackground(0x000000);
    this.gpu.fill(this.startX + 1, this.startY + 1, this.width - 2, this.height - 2, ' ');
  }

  updateScreen() {
    let diff = this.lastBottom - this.currentBottom;
    if (diff == 0) {
      //No change
    } else if (diff > 0) {
      //Scrolling up
      if (diff < this.height - 2) {
        //Copy what we can
        this.gpu.copy(this.startX + 1, this.startY + diff, this.width - 2, (this.height - 2) - diff, 0, diff);
        //render what's left
        for (let row = 0; row < diff; row++) {
          let text = this.buffer[this.currentBottom - (this.height - 2) + row] || '';
          this.gpu.set(this.startX + 1, this.startY + 1 + row - row, text);
          this.gpu.fill(this.startX + 1 + text.length, this.startY + 1 + row, this.width - 2 - text.length, 1, ' ');
        }
      } else {
        //Re-render the whole thing
        for (let row = 0; row < this.height - 2; row++) {
          let text = this.buffer[this.currentBottom - row] || '';
          this.gpu.set(this.startX + 1, this.endY - 1 - row, text);
          this.gpu.fill(this.startX + 1 + text.length, this.endY - 1 - row, this.width - 2 - text.length, 1, ' ');
        }
      }
      this.updateScrollbar()
    } else {
      //Scrolling down
      diff = -diff;
      if (diff < this.height - 2) {
        //Copy what we can
        this.gpu.copy(this.startX + 1, this.startY + 1 + diff, this.width - 2, (this.height - 2) - diff, 0, -diff);
        //render what's left
        for (let row = 0; row < diff; row++) {
          let text = this.buffer[this.currentBottom - row] || '';
          this.gpu.set(this.startX + 1, this.endY - 1 - row, text);
          this.gpu.fill(this.startX + 1 + text.length, this.endY - 1 - row, this.width - 2 - text.length, 1, ' ');
        }
      } else {
        //Re-render the whole thing
        for (let row = 0; row < this.height - 2; row++) {
          let text = this.buffer[this.currentBottom - row] || '';
          this.gpu.set(this.startX + 1, this.endY - 1 - row, text);
          this.gpu.fill(this.startX + 1 + text.length, this.endY - 1 - row, this.width - 2 - text.length, 1, ' ');
        }
      }
      this.updateScrollbar()
    }
    this.lastBottom = this.currentBottom;
  }

  updateScrollbar() {
    let maxTop = this.scrollbackMax - (this.height - 2);
    let top = this.currentBottom - (this.height - 2);
    let percent = (maxTop - top) / (maxTop);
    $bios.log(`percent: ${percent}`);
    //Draw background
    this.gpu.fill(this.endX, this.startY + 1, 1, this.height - 2, '│');
    //Draw bar
    let pos = Math.ceil(this.endY - 1 - ((this.height - 3) * percent));
    this.gpu.set(this.endX, pos, '╪');
  }

  scroll(lines: number) {
    this.currentBottom += lines;
    if (this.currentBottom >= this.scrollbackMax) this.currentBottom = this.scrollbackMax - 1;
    if (this.currentBottom <= this.height - 2) this.currentBottom = this.height - 2;
    this.updateScreen();
  }
}
