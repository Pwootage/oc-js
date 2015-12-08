package com.pwootage.oc.js.api

import java.util

import com.pwootage.oc.js.OCSignalHandler
import li.cil.oc.api.machine.Machine

import scala.collection.JavaConversions._

class JSComputerApi(machine: Machine, sync: OCSignalHandler) {
  def signal():util.Map[String, AnyRef] = {
    if (Thread.interrupted()) {
      throw new InterruptedException("Interrupted; assuming javascript should be shut down")
    }
    sync.pull() match {
      case Some(x) => Map(
        "name" -> x.name(),
        "args" -> x.args()
      )
      case None => null
    }
  }
}
