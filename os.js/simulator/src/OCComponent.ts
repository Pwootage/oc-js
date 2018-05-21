const uuidv4 = require('uuid/v4');

export type OCComponentMethod = (args: any[]) => any;

export interface OCComponentMethodWithDoc {
  fn: OCComponentMethod;
  name: string;
  doc: string;
  direct: boolean;
  limit: number;
  getter: boolean;
  setter: boolean;
}

export abstract class OCComponent {
  readonly id: string;
  methods = new Map<string, OCComponentMethodWithDoc>();

  constructor(public readonly type: string) {
    this.id = uuidv4();
  }

  invoke(method: string, args: any[]): any {
    const f = this.methods.get(method);
    if (f) {
      return f.fn(args);
    } else {
      throw new Error("No such method")
    }
  }

  registerMethod(name: string, fn: OCComponentMethod, opts: Partial<OCComponentMethodWithDoc> = {}) {
    this.methods.set(name, {
      name,
      fn,
      direct: true,
      limit: 0,
      getter: false,
      setter: false,
      doc: `Doc for ${name}`,
      ...opts
    });
  }
}