package com.pwootage.oc.js.spidermonkey

import com.pwootage.oc.js.JSArchitectureBase
import com.pwootage.oc.js.JSEngine
import li.cil.oc.api.machine.Architecture
import li.cil.oc.api.machine.Machine

/**
  * Spidermonkey
  */
@Architecture.Name("ES6 (spidermonkey)")
class SpiderMonkeyArchitecture(_machine: Machine) : JSArchitectureBase(_machine) {

  override fun createEngine(): JSEngine {
    return SpiderMonkeyEngine(this)
  }
}

