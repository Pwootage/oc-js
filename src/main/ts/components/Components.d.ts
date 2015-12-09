declare module components {
  export interface ComponentBase {
    uuid:string;
  }

  export interface EEPROMComponentAPI extends ComponentBase {
    get():string
    set(data:string):void
    getLabel():string
    setLabel(data:string):void
    getSize():number
    getData():string
    setData(data:string):void
    getChecksum():string
    makeReadonly(checksum:string):boolean
  }

  export interface FilesystemComponentAPI extends ComponentBase {
    spaceUsed():number
    open(path:string, mode?:string):number
    seek(handle:number, whence:string, offset:number):number
    makeDirectory(path:string):boolean
    exists(path:string):boolean
    isReadOnly():boolean
    write(handle:number, value:string):boolean
    spaceTotal():number
    isDirectory(path:string):boolean
    rename(from:string, to:string):boolean
    list(path:string):string[]
    lastModified(path:string):number
    getLabel():string
    remove(path:string):boolean
    close(handle:number)
    size(path:string):number
    read(handle:number, count:number):string
    setLabel(value:string):string
  }

  export interface GPUComponent extends ComponentBase {
    bind(address:string):boolean
    getScreen():string
    getBackground():number
    setBackground(color:number, pallete?:boolean):number
    getForeground():number
    setForeground(color:number, pallete?:boolean):number
    getPaletteColor(index:number):number
    getPaletteColor(index:number, value:number):number
    maxDepth():number
    getDepth():number
    setDepth(depth:number):number
    maxResolution():number[]
    getResolution():number[]
    setResolution(width:number, height:number)
    get(x:number, y:number):any[]
    set(x:number, y:number, value:string, vertical?:boolean):boolean
    copy(x:number, y:number, width:number, height:number, tx:number, ty:number): boolean
    fill(x:number, y:number, width:number, height:number, char:string): boolean
  }

  export interface ScreenComponent extends ComponentBase {
    isOn():boolean
    turnOn():boolean
    turnOff():boolean
    getAspectRatio():number[]
    getKeyboards():string[]
    setPrecise(enabled:boolean):boolean
    isPrecise():boolean
    setTouchModeInverted(enabled:boolean):boolean
    isTouchModeInverted():boolean
  }
}