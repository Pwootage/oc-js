package com.pwootage.oc.js

import com.pwootage.oc.js.jsvalue.JSValue
import li.cil.oc.api.machine.Signal

trait JSEngine {
  def started: Boolean
  def start()
  def destroy()
  def evalWithName(filename: String, js: String): JSValue

  def executeThreaded(signal: Option[Signal], synResult: JSValue): RunThreadedResult
  def setMaxMemory(max: Int): Unit
  def maxMemory: Int
  def usedMemory: Int
}
