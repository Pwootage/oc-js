package com.pwootage.oc.js.api

import javax.script.{ScriptException, ScriptEngine}

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

  def compile(name: String, script: String): AnyRef = try scriptEngine.eval(script) catch {
    case e: ScriptException => throw new ScriptException(e.getMessage.replaceAll(" ?in <eval> at line number [0-9]+", ""), name, e.getLineNumber, e.getColumnNumber)
    case e: Throwable => throw e
  }
}
