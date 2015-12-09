interface ScreenComponent extends ComponentBase {
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