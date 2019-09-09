package com.pwootage.oc.js.spidermonkey

import com.pwootage.oc.js._
import li.cil.oc.api.machine.{Architecture, Machine}


/**
  * Spidermonkey
  */
@Architecture.Name("ES6 (spidermonkey)")
class SpiderMonkeyArchitecture(_machine: Machine) extends JSArchitectureBase(_machine) {

  override def createEngine(): JSEngine = {
    new SpiderMonkeyEngine(this)
  }
}
