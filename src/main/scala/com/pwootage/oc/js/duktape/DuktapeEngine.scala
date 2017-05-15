package com.pwootage.oc.js.duktape

import com.pwootage.oc.js.JSEngine
import com.pwootage.oc.js.jsvalue.{JSStringValue, JSUndefined, JSValue}

class DuktapeEngine extends JSEngine {
  private var _maxMemory: Int = 0
  private var allocatedMemory: Int = 0
  private var _started = false

  override def setMaxMemory(max: Int): Unit = _maxMemory = max

  override def maxMemory: Int = _maxMemory

  override def usedMemory: Int = allocatedMemory

  override def started: Boolean = _started

  override def start(): Unit = {
    native_start()
  }

  override def destroy(): Unit = {
    native_destroy()
  }

  override def evalWithName(filename: String, js: String): JSValue = {
    //TODO
    println(s"NEED TO EXECUTE $filename")
    JSUndefined
  }

  override def executeThreaded(syncResult: JSValue): JSValue = {
    println(s"NEED TO EXECUTE THREADED")
    JSUndefined
  }

  //Native methods
  @native def native_start(): Unit

  @native def native_destroy(): Unit

}
