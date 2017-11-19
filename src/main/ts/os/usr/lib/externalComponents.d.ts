export interface ComponentBase {
  uuid: string;
}

export interface EEPROMComponentAPI extends ComponentBase {
  get(): Promise<string>
  set(data: string): Promise<void>
  getLabel(): Promise<string>
  setLabel(data: string): Promise<void>
  getSize(): Promise<number>
  getData(): Promise<string>
  setData(data: string): Promise<void>
  getChecksum(): Promise<string>
  makeReadonly(checksum: string): Promise<boolean>
}

export interface FilesystemComponentAPI extends ComponentBase {
  spaceUsed(): Promise<number>
  open(path: string, mode?: string): Promise<number>
  seek(handle: number, whence: string, offset: number): Promise<number>
  makeDirectory(path: string): Promise<boolean>
  exists(path: string): Promise<boolean>
  isReadOnly(): Promise<boolean>
  write(handle: number, value: string): Promise<boolean>
  spaceTotal(): Promise<number>
  isDirectory(path: string): Promise<boolean>
  rename(from: string, to: string): Promise<boolean>
  list(path: string): Promise<string[]>
  lastModified(path: string): Promise<number>
  getLabel(): Promise<string>
  remove(path: string): Promise<boolean>
  close(handle: number): Promise<void>
  size(path: string): Promise<number>
  read(handle: number, count: number): Promise<string>
  setLabel(value: string): Promise<string>
}

export interface GPUComponent extends ComponentBase {
  bind(address: string): Promise<boolean>
  getScreen(): Promise<string>
  getBackground(): Promise<number>
  setBackground(color: number, pallete?: boolean): Promise<number>
  getForeground(): Promise<number>
  setForeground(color: number, pallete?: boolean): Promise<number>
  getPaletteColor(index: number): Promise<number>
  getPaletteColor(index: number, value: number): Promise<number>
  maxDepth(): Promise<number>
  getDepth(): Promise<number>
  setDepth(depth: number): Promise<number>
  maxResolution(): Promise<number[]>
  getResolution(): Promise<number[]>
  setResolution(width: number, height: number): Promise<void>
  get(x: number, y: number): Promise<any[]>
  set(x: number, y: number, value: string, vertical?: boolean): Promise<boolean>
  copy(x: number, y: number, width: number, height: number, tx: number, ty: number): Promise<boolean>
  fill(x: number, y: number, width: number, height: number, char: string): Promise<boolean>
}

export interface ScreenComponent extends ComponentBase {
  isOn(): Promise<boolean>
  turnOn(): Promise<boolean>
  turnOff(): Promise<boolean>
  getAspectRatio(): Promise<number[]>
  getKeyboards(): Promise<string[]>
  setPrecise(enabled: boolean): Promise<boolean>
  isPrecise(): Promise<boolean>
  setTouchModeInverted(enabled: boolean): Promise<boolean>
  isTouchModeInverted(): Promise<boolean>
}
