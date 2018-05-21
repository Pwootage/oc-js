import * as vm from 'vm';
import {OCSandbox} from "./OCSandbox";
import * as fs from "fs";
import * as path from "path";
import {EEPROMComponent} from "./components/EEPROMComponent";
import {FilesystemComponent} from "./components/FilesystemComponent";

export function main(args: string[]) {
  const context = new OCSandbox();
  context.registerComponent(new EEPROMComponent(path.resolve(process.cwd(), 'eeprom.js')));
  context.registerComponent(new FilesystemComponent(path.resolve(process.cwd(), 'root')));

  let biosPath = path.resolve(process.cwd(), 'bios.js');
  if (!fs.existsSync(biosPath)) {
    biosPath = path.resolve(__dirname, '../../src/main/resources/assets/oc-js/bios/bios.js');
  }
  if (!fs.existsSync(biosPath)) {
    biosPath = path.resolve(__dirname, '../../../src/main/resources/assets/oc-js/bios/bios.js');
  }
  const biosSrc: string = fs.readFileSync(biosPath, {encoding: 'utf8'});
  context.biosCompile(
    '__bios__',
    `(function(exports, global){${biosSrc}
            throw new Error("<bios ended execution>");
         })({}, this);`);
}
