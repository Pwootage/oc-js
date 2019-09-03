package com.pwootage.oc.js.duktape

import com.pwootage.oc.js._
import li.cil.oc.api.machine.{Architecture, Machine}


/**
  * Spidermonkey
  */
@Architecture.Name("ES5 (duktape)")
class DuktapeArchitecture(_machine: Machine) extends JSArchitectureBase(_machine) {

  override def createEngine(): JSEngine = {
    new DuktapeEngine(this)
  }

  override def setupSandbox(): Unit = {
    //TODO
  }
}
