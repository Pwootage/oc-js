package com.pwootage.oc.js.api

import com.pwootage.oc.js.jsvalue.JSValue
import com.pwootage.oc.js.{JSEngine, OCJS}
import li.cil.oc.api.machine.Machine

class JSBiosInternalAPI(machine: Machine, scriptEngine: JSEngine) {
  def log(msg: String): Unit = {
    // No-op for speeeed
    OCJS.log.error(msg)
  }

  def crash(msg: String): Unit = {
    machine.crash(msg)
    throw new JSExitException("JS Crashed: " + msg)
  }
}
