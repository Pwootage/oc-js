package com.pwootage.oc.js.api

import java.util

import com.pwootage.oc.js.OCSignalHandler
import li.cil.oc.api.machine.Machine
import li.cil.oc.api.network.Connector

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

  //TODO: Push signal!

  def address():String = machine.node().address()

  def tmpAddress():String = machine.tmpAddress()

  // THE MEMORY IS A LIE
  def freeMemory():Int = 2 * 1024 * 1024

  def totalMemory():Int = 4 * 1024 * 1024

  def energy():Double = machine.node.asInstanceOf[Connector].globalBuffer

  def maxEnergy():Double = machine.node.asInstanceOf[Connector].globalBufferSize

  def uptime():Double = machine.upTime()

  //TODO: Handle users eventually
}
