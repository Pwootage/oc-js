interface GPUComponent extends ComponentBase {
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