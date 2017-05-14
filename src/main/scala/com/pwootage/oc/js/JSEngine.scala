package com.pwootage.oc.js

trait JSEngine {
  def started: Boolean
  def start()
  def destroy()
  def evalWithName(filename: String, js: String): AnyRef

  def executeThreaded(synResult: Option[Any]): AnyRef
  def executeSync(): Unit
  def setMaxMemory(max: Int): Unit
  def maxMemory: Int
  def usedMemory: Int
}
