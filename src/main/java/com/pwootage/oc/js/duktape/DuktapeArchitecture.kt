package com.pwootage.oc.js.duktape

import com.pwootage.oc.js.JSArchitectureBase
import com.pwootage.oc.js.JSEngine
import li.cil.oc.api.machine.Architecture
import li.cil.oc.api.machine.Machine


/**
 * Spidermonkey
 */
@Architecture.Name("ES5 (duktape)")
class DuktapeArchitecture(_machine: Machine) : JSArchitectureBase(_machine) {

  override fun createEngine(): JSEngine {
    return DuktapeEngine(this)
  }
}
