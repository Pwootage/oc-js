package com.pwootage.oc.js

import com.pwootage.oc.js.jsvalue.JSValue
import com.pwootage.oc.js.v8.V8ExecutionContext

trait JSEngine {
  def started: Boolean
  def start()
  def destroy()
  def evalWithName(filename: String, js: String, context: V8ExecutionContext): JSValue

  def executeThreaded(synResult: JSValue): JSValue
  def setMaxMemory(max: Int): Unit
  def maxMemory: Int
  def usedMemory: Int
}
