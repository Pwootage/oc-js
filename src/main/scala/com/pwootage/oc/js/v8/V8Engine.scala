package com.pwootage.oc.js.v8

import java.nio.ByteBuffer

import com.google.common.io.ByteStreams
import com.pwootage.oc.js.JSEngine
import com.pwootage.oc.js.jsvalue.{JSNull, JSStringValue, JSValue}

class V8Engine extends JSEngine {
  private var v8EngineNative: Long = 0
  private var _maxMemory: Int = 0
  private var allocatedMemory: Int = 0
  private var _started = false

  // Initialize on startup
  if (!V8Static.isInitialized) {
    V8Static.initialize()
  }
  native_start()

  override def setMaxMemory(max: Int): Unit = _maxMemory = max

  override def maxMemory: Int = _maxMemory

  override def usedMemory: Int = allocatedMemory

  override def started: Boolean = _started

  override def start(): Unit = {
  }

  override def destroy(): Unit = {
    native_destroy()
  }

  override def evalWithName(filename: String, js: String, context: V8ExecutionContext): JSValue = {
    val res = compile_and_execute(js, filename, context.raw)
    JSValue.fromJSON(res)
  }

  override def executeThreaded(syncResult: JSValue): JSValue = {
    println(s"NEED TO EXECUTE THREADED")
    JSNull
  }

  //Native methods
  @native protected def native_start(): Unit

  @native protected def native_destroy(): Unit

  @native protected def compile_and_execute(src: String, filename: String, context: Int): String
}