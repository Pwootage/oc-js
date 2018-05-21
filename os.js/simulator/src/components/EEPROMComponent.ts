import {OCComponent} from "../OCComponent";
import * as fs from 'fs';

export class EEPROMComponent extends OCComponent {
  maxSize = 4096;
  maxDataSize = 256;

  constructor(public filePath: string, public label: string = 'eeprom', public data: string = '') {
    super('eeprom');

    // Register all our methods
    this.registerMethod('get', () => {
      return fs.readFileSync(this.filePath, {encoding: 'utf-8'});
    });
    this.registerMethod('set', (args) => {
      const arg = args[0];
      fs.writeFileSync(
        this.filePath,
        arg.substring(0, Math.min(this.maxSize, arg.length))
      );
    });
    this.registerMethod('getLabel', () => {
      return this.label;
    });
    this.registerMethod('getSize', () => {
      return this.maxSize;
    });
    this.registerMethod('getDataSize', () => {
      return this.maxDataSize;
    });
    this.registerMethod('getData', () => {
      return this.data;
    });
    this.registerMethod('setData', (args) => {
      const arg = args[0];
      this.data = arg.substring(0, Math.min(this.maxDataSize, arg.length));
    });
    this.registerMethod('getChecksum', () => {
      throw new Error("Not yet implemented in simulator");
    });
    this.registerMethod('makeReadonly', () => {
      throw new Error("Not yet implemented in simulator");
    });
  }
}