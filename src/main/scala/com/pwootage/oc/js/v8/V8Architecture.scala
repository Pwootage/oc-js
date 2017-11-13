package com.pwootage.oc.js.v8

import com.pwootage.oc.js._
import li.cil.oc.api.machine.{Architecture, Machine}

/**
  * Nashorn Arch
  */
@Architecture.Name("ES6 (v8)")
class V8Architecture(_machine: Machine) extends JSArchitectureBase(_machine) {

  override def createEngine(): JSEngine = {
    new V8Engine()
  }

  override def setupSandbox(): Unit = {
    //TODO
  }
}
