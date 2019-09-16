//package com.pwootage.oc.js.spidermonkey
//
//import com.pwootage.oc.js.*
//
//class SpiderMonkeyEngine(val arch: SpiderMonkeyArchitecture) : JSEngine {
//  private var spiderMonkeyEngineNative: Long = 0
//  private var _maxMemory: Int = 0
//  private var allocatedMemory: Int = 0
//  private var _started = false
//
//  // Initialize on startup
//  init {
//    if (!SpiderMonkeyStatic.isInitialized) {
//      SpiderMonkeyStatic.initialize()
//    }
//    native_start()
//  }
//
//  override fun setMaxMemory(max: Int) {
//    _maxMemory = max
//  }
//
//  override val maxMemory get() = _maxMemory
//
//  override val usedMemory get() = allocatedMemory
//
//  override val started get() = _started
//
//  override fun start() {
//    _started = true
//    val biosJS = StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js")
//    val biosSrc =
//      """(function(exports, global){try {$biosJS
//           return "<bios ended execution>";
//         /**/} catch (error) {
//           const backup = new Error('<no error somehow>');
//           return error.stack || backup.stack || '<no stack>';
//         }
//         })({}, this);
//        """
//    arch.componentInvoker.setResult(JSStringValue(biosSrc))
//  }
//
//  override fun destroy() {
//    native_destroy()
//  }
//
//  //Native methods
//  private external fun native_start(): Unit
//
//  private external fun native_destroy(): Unit
//
//  private external fun native_next(next: String): String
//}