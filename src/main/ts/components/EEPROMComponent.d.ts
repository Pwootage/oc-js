interface EEPROMComponentAPI extends ComponentBase {
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