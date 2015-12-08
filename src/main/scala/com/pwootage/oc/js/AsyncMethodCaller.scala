package com.pwootage.oc.js

import scala.concurrent._

class AsyncMethodCaller {
  private var callToSync: Option[() => Any] = None
  private var callPromise: Option[Promise[Any]] = None
  private var callResult: Option[Any] = None

  //TODO: Sanity checks

  def callSync[T](fn: () => T): Future[T] = {
    this.callToSync = Some(fn)
    this.callPromise = Some(Promise())
    this.callPromise.get.future.asInstanceOf[Future[T]] //Cheating, but this is actually a safe call
  }

  def executeSync() = {
    callToSync match {
      case Some(x) => callResult = Some(x())
      case _ =>
    }
    callToSync = None
  }

  def resolveSync(): Unit = {
    callPromise match {
      case Some(x) => x.success(callResult.get)
      case _ =>
    }
    callPromise = None
    callResult = None
  }

  def outstandingSync = this.callToSync.isDefined
}
