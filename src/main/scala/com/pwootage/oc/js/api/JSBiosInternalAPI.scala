package com.pwootage.oc.js.api

import javax.script.ScriptEngine

import com.pwootage.oc.js.OCJS
import li.cil.oc.api.machine.Machine

class JSBiosInternalAPI(machine: Machine, scriptEngine: ScriptEngine) {
  def log(msg: String): Unit = {
    OCJS.log.error(msg)
  }

  def crash(msg: String): Unit = {
    machine.crash(msg)
    throw new JSExitException("JS Crashed: " + msg)
  }

  def compile(script: String): Unit = {
    scriptEngine.eval(script)
  }
}
