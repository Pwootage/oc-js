import * as vm from 'vm';
import {sleep} from 'sleep';
import {
  __compileFunction,
  __yieldFunction,
  BiosYield,
  BiosYieldCall,
  BiosYieldResult
} from './biosDefinitions';
import {OCComponent} from "./OCComponent";

interface OCContext extends vm.Context {
  __yield: __yieldFunction;
  __compile: __compileFunction;
}

export class OCSandbox {
  sandbox: OCContext;
  components = new Map<string, OCComponent>();

  constructor() {
    this.sandbox = {
      __yield: (req) => JSON.stringify(this.biosYield(req)),
      __compile: (file, src) => this.biosCompile(file, src)
    };
    vm.createContext(this.sandbox);
  }

  biosYield(req: BiosYield): BiosYieldResult {
    if (req.type == 'call') {
      return this.biosCall(req);
    } else if (req.type == 'sleep') {
      sleep(req.duration);
      return {
        state: 'success',
        value: null //TODO: handle signal
      };
    } else {
      return {
        state: 'error',
        value: `Unknown bios yield type ${(req as any).type}`
      }
    }
  }

  biosCompile(file: string, src: string): any {
    return vm.runInContext(src, this.sandbox, {
      filename: file,
      displayErrors: true
    });
  }

  private biosCall(req: BiosYieldCall): BiosYieldResult {
    if (req.name == 'bios.crash') {
      throw new Error(`Bios crash: ${req.args.join(', ')}`)
    } else if (req.name == 'bios.log') {
      console.log(...req.args);
      return {state: 'success', value: null};
    } else if (req.name == 'component.invoke') {
      return this.invoke(req.args[0], req.args[1], req.args[2]);
    } else if (req.name == 'component.list') {
      const res: any[] = [];
      for (const [key, value] of this.components) {
        res.push({uuid: key, type: value.type});
      }
      return {state: 'success', value: res}
    } else {
      return {state: 'error', value: `Unknown call ${req.name}`}
    }
  }

  private invoke(address: string, method: string, args: any[]): BiosYieldResult {
    const component = this.components.get(address);
    if (!component) {
      return {state: 'error', value: `Unknown component ${address}`};
    }
    try {
      const res = component.invoke(method, args);
      return {state: 'success', value: res};
    } catch (e) {
      return {state: 'error', value: `Error invoking method ${method}: ${e}`};
    }
  }
}