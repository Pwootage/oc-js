package com.pwootage.oc.js

import java.io.Reader
import javax.script.{Invocable, ScriptContext, ScriptEngine}

import com.pwootage.oc.js.api.JSExitException
import li.cil.oc.api.machine.{Signal, Machine}

import scala.concurrent.Promise

class NashornExecutionThread(machine: Machine, se: ScriptEngine, r: (ScriptEngine) => Unit) extends Thread {
  private var _jsRunning = true
  private var _started = false
  private var _exception: Option[Throwable] = None
  val runSyncMethodCaller = new AsyncMethodCaller
  val signalHandler = new OCSignalHandler

  override def run(): Unit = {
    _started = true;
    try r(se) catch {
      case e: Throwable => _exception = Some(e)
    } finally {
      _jsRunning = false
    }
    _exception match {
      case Some(e: JSExitException) => machine.crash(e.getMessage)
      case Some(e: RuntimeException) => //Everything's fine
        e.getCause match {
          case e: JSExitException => machine.crash(e.getMessage)
          case _: InterruptedException => //Everything's fine (we just want to shut down JS)
          case _ =>
            machine.crash("JS error: " + e.toString)
            OCJS.log.error("Error in JS thread", e)
        }
      case _ => machine.crash("JS ended execution.")
    }

  }

  /**
    * @return true if javascript is still running
    */
  def jsRunning = _jsRunning

  def started = _started

  def exception = _exception

  def beNiceKill() = {
    this.interrupt()
    val t = this
    new Thread(new Runnable {
      override def run(): Unit = {
        Thread.sleep(10000)
        if (t._jsRunning) {
          OCJS.log.error("Forcibly killing a javascript thread")
          t.stop()
        }
      }
    }).start()
  }
}
