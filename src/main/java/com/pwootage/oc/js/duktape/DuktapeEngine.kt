package com.pwootage.oc.js.duktape

import com.pwootage.oc.js.*

class DuktapeEngine(val arch: DuktapeArchitecture) : JSEngine(arch) {
  private var duktapeEngineNative: Long = 0
  private var _started = false

  // Initialize on startup
  init {
    if (!DuktapeStatic.isInitialized) {
      DuktapeStatic.initialize()
    }
    native_start()
  }

  override fun setMaxMemory(max: Int) {
    native_set_max_memory(max)
  }

  override val maxMemory get() = native_get_max_memory()

  override val usedMemory get() = native_get_available_memory()

  override val started get() = _started

  override fun start() {
    _started = true
    val biosJS = StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js")
    val shim = StaticJSSrc.loadSrc("/assets/oc-js/es6-shim.min.js")
    val biosSrc =
      """$shim;(function(exports, global){try {$biosJS
           return "<bios ended execution>";
         /**/} catch (error) {
           const backup = new Error('<no error somehow>');
           return error.stack || backup.stack || '<no stack>';
         }
         })({}, this);
        """
    arch.componentInvoker.setResult(JSStringValue(biosSrc))
  }

  override fun destroy() {
    native_destroy()
  }

  override fun jsPushNext(jsNext: JSValue): JSValue = native_next(jsNext)

  //Native methods
  private external fun native_start(): Unit

  private external fun native_destroy(): Unit

  private external fun native_next(next: JSValue): JSValue

  private external fun native_set_max_memory(max: Int)

  private external fun native_get_max_memory(): Int

  private external fun native_get_available_memory(): Int
}