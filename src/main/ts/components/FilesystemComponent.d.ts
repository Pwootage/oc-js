interface FilesystemComponentAPI extends ComponentBase {
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