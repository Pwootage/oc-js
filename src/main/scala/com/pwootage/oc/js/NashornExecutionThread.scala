package com.pwootage.oc.js

import java.io.Reader
import javax.script.{Invocable, ScriptContext, ScriptEngine}

import li.cil.oc.api.machine.Machine

class NashornExecutionThread(machine: Machine, se: ScriptEngine, r: (ScriptEngine) => Unit) extends Thread {
  private var _jsRunning = true
  private var _exception: Option[Throwable] = None

  override def run(): Unit = {
    try r(se) catch {
      case e: Throwable => _exception = Some(e)
    } finally {
      _jsRunning = false
    }
    machine.crash(_exception.map(_.toString).getOrElse("Javascript ended execution"))
  }

  /**
    * @return true if javascript is still running
    */
  def jsRunning = _jsRunning

  def exception = _exception
}
