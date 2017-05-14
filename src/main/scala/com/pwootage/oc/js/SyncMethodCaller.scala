package com.pwootage.oc.js

class SyncMethodCaller {
  private var callToSync: Option[() => Any] = None
  private var callResult: Option[Any] = None

  def hasSyncCall: Boolean = this.callToSync.isDefined
  def hasResult: Boolean = this.callResult.isDefined

  def callSync[T](fn: () => T): Unit = {
    this.callToSync = Some(fn)
  }

  def executeSync(): Unit = {
    callToSync match {
      case Some(x) => callResult = Some(x())
      case _ =>
    }
    callToSync = None
  }

  def result(): Option[Any] = {
    val res = callResult
    callResult = None
    res
  }
}
