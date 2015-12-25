package com.pwootage.oc.js.api

import javax.script.{ScriptContext, ScriptException, ScriptEngine}

import com.pwootage.oc.js.OCJS
import li.cil.oc.api.machine.Machine

object CompileCounter {
  var count = 0
}

class JSBiosInternalAPI(machine: Machine, scriptEngine: ScriptEngine) {
  def log(msg: String): Unit = {
    OCJS.log.error(msg)
  }

  def crash(msg: String): Unit = {
    machine.crash(msg)
    throw new JSExitException("JS Crashed: " + msg)
  }

  def compile(name: String, script: String): AnyRef = {
    scriptEngine.getContext.setAttribute(ScriptEngine.FILENAME, name, ScriptContext.ENGINE_SCOPE)
    scriptEngine.eval(script)
  }
}
