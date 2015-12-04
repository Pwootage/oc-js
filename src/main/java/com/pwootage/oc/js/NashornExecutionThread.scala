package com.pwootage.oc.js

import java.io.Reader
import javax.script.{ScriptContext, ScriptEngine}

class NashornExecutionThread(se: ScriptEngine, script: Reader) extends Thread {
  private var _jsRunning = true
  override def run(): Unit = {
    se.eval(script)
    _jsRunning = false
  }

  /**
    * @return true if javascript is still running
    */
  def jsRunning = _jsRunning
}
