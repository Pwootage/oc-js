package com.pwootage.oc.js

abstract class DuktapeEngine extends JSEngine {
  private var _maxMemory: Long = 0
  private var allocatedMemory: Long = 0
  private var _started = false

  override def setMaxMemory(max: Int): Unit = _maxMemory = max

  override def started: Boolean = _started

  override def start(): Unit = {
    native_start()
  }

  override def destroy(): Unit = {
    native_destroy()
  }

  override def evalWithName(filename: String, js: String): AnyRef = {
    //TODO
    println(s"NEED TO EXECUTE $filename")
    "TODO"
  }

  override def executeThreaded(synResult: Option[Any]): Unit = {
    println(s"NEED TO EXECUTE THREADED")
  }

  override def executeSync(): Unit = {
    println(s"NEED TO EXECUTE SYNC")
  }

  //Native methods
  @native def native_start(): Unit

  @native def native_destroy(): Unit

}
