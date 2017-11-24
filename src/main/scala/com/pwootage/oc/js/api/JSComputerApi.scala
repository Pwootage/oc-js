package com.pwootage.oc.js.api

import java.util

import com.pwootage.oc.js.{JSEngine, JSUtils, OCSignalHandler}
import li.cil.oc.api.machine.Machine
import li.cil.oc.api.network.Connector

import scala.collection.JavaConversions._

class JSComputerApi(machine: Machine, engine: JSEngine) {
  def address(): String = machine.node().address()

  def tmpAddress(): String = machine.tmpAddress()

  def freeMemory(): Int = engine.maxMemory - engine.usedMemory

  def totalMemory(): Int = engine.maxMemory

  def energy(): Double = machine.node.asInstanceOf[Connector].globalBuffer

  def maxEnergy(): Double = machine.node.asInstanceOf[Connector].globalBufferSize

  def uptime(): Double = machine.upTime()

  //TODO: Handle users eventually
}
