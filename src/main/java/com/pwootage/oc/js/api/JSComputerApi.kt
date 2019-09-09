package com.pwootage.oc.js.api

import com.pwootage.oc.js.JSEngine
import li.cil.oc.api.machine.Machine
import li.cil.oc.api.network.Connector


class JSComputerApi(val machine: Machine, val engine: JSEngine) {
  fun address(): String = machine.node().address()

  fun tmpAddress(): String = machine.tmpAddress()

  fun freeMemory(): Int = engine.maxMemory - engine.usedMemory

  fun totalMemory(): Int = engine.maxMemory

  fun energy(): Double = (machine.node() as Connector).globalBuffer()

  fun maxEnergy(): Double = (machine.node() as Connector).globalBufferSize()

  fun uptime(): Double = machine.upTime()

  //TODO: Handle users eventually
}
