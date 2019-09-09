package com.pwootage.oc.js

class ComponentInvoker {
  private var callToSync: (() -> JSValue)? = null
  private var callResult: JSValue? = null

  val hasSyncCall: Boolean get() = this.callToSync != null
  val hasResult: Boolean get() = this.callResult != null

  fun callSync(fn: () -> JSValue): Unit {
    this.callToSync = fn
  }

  fun executeSync(): Unit {
    if (callToSync != null) {
      callResult = callToSync?.invoke()
      callToSync = null
    }
  }

  fun setResult(v: JSValue): Unit {
    callResult = v
  }

  fun result(): JSValue? {
    val res = callResult
    callResult = null
    return res
  }
}
