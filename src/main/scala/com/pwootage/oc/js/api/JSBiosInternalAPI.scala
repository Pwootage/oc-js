package com.pwootage.oc.js.api

import com.pwootage.oc.js.jsvalue.JSValue
import com.pwootage.oc.js.{JSEngine, OCJS}
import li.cil.oc.api.machine.Machine

class JSBiosInternalAPI(machine: Machine, scriptEngine: JSEngine) {
  def log(msg: String): Unit = {
    OCJS.log.error(msg)
  }

  def crash(msg: String): Unit = {
    machine.crash(msg)
    throw new JSExitException("JS Crashed: " + msg)
  }

  def compile(name: String, script: String): JSValue = {
    //TODO: Allow this to set context?
    scriptEngine.evalWithName(name, script)
  }
}
