"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
Object.defineProperty(global, 'global', {
    enumerable: false,
    value: global,
    writable: false,
    configurable: false
});
function biosYield(call) {
    const biosResJson = __yield(call);
    const result = JSON.parse(biosResJson);
    if (!result) {
        return result;
    }
    else if (result.state == 'success') {
        return result.value;
    }
    else {
        throw new Error(`Error throw in java from call ${JSON.stringify(call)}: ${JSON.stringify(result)}`);
    }
}
class BiosComponentApiImpl {
    list(filter) {
        const arr = biosYield({
            type: 'call',
            name: 'component.list',
            args: ['']
        });
        if (filter) {
            if (filter instanceof RegExp) {
                return arr.filter(v => !!v.type.match(filter));
            }
            else {
                return arr.filter(v => v.type === filter);
            }
        }
        else {
            return arr;
        }
    }
    invoke(address, name, ...args) {
        const res = biosYield({
            type: 'call',
            name: 'component.invoke',
            args: [address, name, args]
        });
        if (!res || res.length == 0) {
            return undefined;
        }
        else if (res.length == 1) {
            return res[0];
        }
        else {
            return res;
        }
    }
    doc(address, name) {
        return biosYield({
            type: 'call',
            name: 'component.doc',
            args: [address, name]
        });
    }
    methods(address) {
        return biosYield({
            type: 'call',
            name: 'component.methods',
            args: [address]
        });
    }
    type(address) {
        return biosYield({
            type: 'call',
            name: 'component.type',
            args: [address]
        });
    }
    proxy(address) {
        const res = {};
        const methods = this.methods(address);
        if (!methods)
            return null;
        Object.defineProperty(res, 'uuid', {
            enumerable: true,
            value: address
        });
        for (const name in methods) {
            const method = methods[name];
            let g = (n) => () => this.invoke.apply(this, [address, n]);
            let s = (n, v) => this.invoke.apply(this, [address, n, v]);
            if (method.getter || method.setter) {
                Object.defineProperty(res, method.name, {
                    enumerable: true,
                    'get': method.getter ? g.bind(this, method.name) : null,
                    'set': method.setter ? s.bind(this, method.name) : null
                });
            }
            else {
                let i = (n, ...args) => this.invoke.apply(this, [address, n].concat(args));
                Object.defineProperty(res, method.name, {
                    enumerable: true,
                    writable: false,
                    value: i.bind(this, method.name)
                });
            }
        }
        return res;
    }
    first(type) {
        const list = this.list(type);
        const address = list[0] && list[0].uuid;
        if (!address) {
            return null;
        }
        else {
            return $bios.component.proxy(address);
        }
    }
}
class BiosComputerApiImpl {
    sleep(duration) {
        const signal = biosYield({
            type: 'sleep',
            duration
        });
        if (signal) {
            $bios.signals.emit('signal', signal);
            $bios.signals.emit(signal.name, signal.args);
        }
        return signal;
    }
    address() {
        return biosYield({
            type: 'call',
            name: 'computer.address',
            args: []
        });
    }
    tmpAddress() {
        return biosYield({
            type: 'call',
            name: 'computer.tmpAddress',
            args: []
        });
    }
    freeMemory() {
        return biosYield({
            type: 'call',
            name: 'computer.freeMemory',
            args: []
        });
    }
    totalMemory() {
        return biosYield({
            type: 'call',
            name: 'computer.totalMemory',
            args: []
        });
    }
    energy() {
        return biosYield({
            type: 'call',
            name: 'computer.energy',
            args: []
        });
    }
    maxEnergy() {
        return biosYield({
            type: 'call',
            name: 'computer.maxEnergy',
            args: []
        });
    }
    uptime() {
        return biosYield({
            type: 'call',
            name: 'computer.uptime',
            args: []
        });
    }
}
class EventEmitter {
    constructor() {
        this.listeners = new Map();
    }
    on(name, fn) {
        let arr = this.listeners.get(name) || [];
        arr.push(fn);
        this.listeners.set(name, arr);
        return this.deregister.bind(this, name, fn);
    }
    emit(name, ...args) {
        (this.listeners.get(name) || []).forEach(v => v(...args));
    }
    deregister(name, fn) {
        let arr = this.listeners.get(name) || [];
        arr = arr.filter(v => v !== fn);
        if (arr.length == 0) {
            this.listeners.delete(name);
        }
        else {
            this.listeners.set(name, arr);
        }
    }
}
global.EventEmitter = EventEmitter;
class BiosApiImpl {
    constructor() {
        this.component = new BiosComponentApiImpl();
        this.computer = new BiosComputerApiImpl();
        this.signals = new EventEmitter();
    }
    crash(msg) {
        const gpu = $bios.component.first('gpu');
        const screen = $bios.component.first('screen');
        if (gpu && screen) {
            gpu.bind(screen.uuid);
            gpu.setBackground(0x0000FF, false);
        }
        return biosYield({
            type: 'call',
            name: 'bios.crash',
            args: [msg]
        });
    }
    compile(filename, script) {
        return __compile(filename, script);
    }
    log(message) {
        biosYield({
            type: 'call',
            name: 'bios.log',
            args: [message]
        });
    }
}
let biosImpl = new BiosApiImpl();
global.$bios = biosImpl;
let eeprom = $bios.component.first('eeprom');
if (!eeprom) {
    $bios.crash('No eeprom!');
}
else {
    let eepromSrc = eeprom.get();
    try {
        $bios.compile('eeprom', `(function(global, exports, define){${eepromSrc}
    })(global, {});`);
        throw new Error('EEPROM ended execution');
    }
    catch (error) {
        $bios.crash('Failed to compile eeprom: ' + error.stack.toString());
    }
}
