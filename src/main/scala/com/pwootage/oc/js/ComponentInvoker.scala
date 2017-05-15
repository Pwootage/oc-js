package com.pwootage.oc.js

import com.pwootage.oc.js.jsvalue.JSValue

class ComponentInvoker {
  private var callToSync: Option[() => JSValue] = None
  private var callResult: Option[JSValue] = None

  def hasSyncCall: Boolean = this.callToSync.isDefined
  def hasResult: Boolean = this.callResult.isDefined

  def callSync(fn: () => JSValue): Unit = {
    this.callToSync = Some(fn)
  }

  def executeSync(): Unit = {
    callToSync match {
      case Some(x) => callResult = Some(x())
      case _ =>
    }
    callToSync = None
  }

  def setResult(v: JSValue): Unit = {
    callResult = Some(v)
  }

  def result(): Option[JSValue] = {
    val res = callResult
    callResult = None
    res
  }
}
