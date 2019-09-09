package com.pwootage.oc.js.duktape

object DuktapeStatic {
  var isInitialized = false
    private set

  fun initialize() {
    synchronized(this) {
      if (isInitialized) {
        return
      }
      println("Initializing Duktape")
      isInitialized = true
      native_init()
      println("SpiderMonkey Duktape")
    }
  }

  private external fun native_init()
}
