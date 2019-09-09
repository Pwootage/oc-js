package com.pwootage.oc.js

interface JSEngine {
  val started: Boolean
  fun start()
  fun destroy()

  fun executeThreaded(jsNext: JSValue): RunThreadedResult
  fun setMaxMemory(max: Int): Unit
  val maxMemory: Int
  val usedMemory: Int
}
