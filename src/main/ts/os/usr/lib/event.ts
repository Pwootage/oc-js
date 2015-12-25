
export interface DeregFunction {
  ()
}

type EventHandler = Function;

export class EventEmitter {
  private listeners: { [key:string]:EventHandler[] }

  constructor() {}

  on(name:string, fn: EventHandler):DeregFunction {
    this.listeners[name] = this.listeners[name] || [];
    this.listeners[name].push(fn);
    return this.deregister.bind(this, fn);
  }

  emit(name:string, ...args:any[]) {
    (this.listeners[name] || []).forEach(v => v(args));
  }

  deregister(fn: EventHandler) {
    this.listeners[name] = (this.listeners[name] || []).filter(v => v !== fn);
    if (this.listeners[name].length == 0) delete this.listeners[name];
  }
}
