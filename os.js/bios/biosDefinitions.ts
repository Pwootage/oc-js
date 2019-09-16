export interface ComponentInvokeParams {
  address: string,
  name: string,
  args: any[]
}

export type BiosYieldParams = [ComponentInvokeParams] | any[];
export type BiosYield = BiosYieldCall | BiosYieldSleep;
export type BiosCallName =
  'component.invoke' |
  'component.list' |
  'component.doc' |
  'component.methods' |
  'component.type' |
  'computer.address' |
  'computer.tmpAddress' |
  'computer.freeMemory' |
  'computer.totalMemory' |
  'computer.energy' |
  'computer.maxEnergy' |
  'computer.uptime' |
  'bios.crash' |
  'bios.log';

export interface BiosYieldCall {
  type: 'call';
  name: BiosCallName;
  args: BiosYieldParams;
}

export interface BiosYieldSleep {
  type: 'sleep';
  duration: number;
}

export interface BiosYieldResult {
  state: 'error' | 'success';
  value: any;
}

export type __yieldFunction = (call: BiosYield) => BiosYieldResult;
export type __compileFunction = (file: string, src: string) => any;