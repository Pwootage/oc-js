package com.pwootage.oc.js.duktape

import java.lang.Iterable
import java.util
import javax.script.{Invocable, ScriptContext, ScriptEngine}

import com.pwootage.oc.js.api.{JSBiosInternalAPI, JSComponentApi, JSComputerApi}
import com.pwootage.oc.js.{NashornExecutionThread, OCJS, StaticJSSrc}
import jdk.nashorn.api.scripting.{ClassFilter, NashornScriptEngineFactory}
import li.cil.oc.api.machine.{Architecture, ExecutionResult, Machine}
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound

import scala.concurrent._

/**
  * Nashorn Arch
  */
@Architecture.Name("ES5 (duktape)")
class DuktapeArchitecture(val machine: Machine) extends Architecture {
  private var _initialized = false
  private var _started = false
  private var connected: Promise[Unit] = Promise()

  private var sem: NashornScriptEngineFactory = null
  private var mainEngine: ScriptEngine = null
  private var executionThread: NashornExecutionThread = null

  override def isInitialized: Boolean = _initialized

  override def onConnect(): Unit = {
    connected.synchronized {
      if (!connected.isCompleted) connected.success(Unit)
    }
  }

  override def onSignal(): Unit = {
  }

  override def initialize(): Boolean = {
    try {
      if (_initialized) return _initialized
      //Check if it's already connected (onConnect doesn't get called, otherwise
      connected.synchronized {
        if (!connected.isCompleted && machine.node() != null && machine.node().network() != null) connected.success(Unit)
      }

      sem = new NashornScriptEngineFactory()
      mainEngine = sem.getScriptEngine(Array("-strict", "--no-java", "--no-syntax-extensions"),
        Thread.currentThread().getContextClassLoader,
        new ClassFilter {
          override def exposeToScripts(s: String): Boolean = s.startsWith("com.pwootage.oc.nashorn.api")
        })
      //Define 'glal' object
      mainEngine.eval("var global = this;")
      ///Delete a bunch of crap
      val bindings = mainEngine.getBindings(ScriptContext.ENGINE_SCOPE)
      bindings.remove("load")
      bindings.remove("loadWithNewGlobal")
      bindings.remove("print")
      bindings.remove("printf")
      bindings.remove("sprintf")
      bindings.remove("println")
      bindings.remove("exit") //Seriously, Nashorn? Seriously?
      bindings.remove("quit")
      bindings.remove("engine")
      bindings.remove("context")
      bindings.remove("factory")
      //The remove doesn't really work for everything, so we do this too
      //They show up as null (not undefined) but arn't accessible
      bindings.put("engine", null)
      bindings.put("context", null)
      bindings.put("factory", null)
      bindings.put("arguments", null)

      //Load bios
      mainEngine.getContext.setAttribute(ScriptEngine.FILENAME, "bios.js", ScriptContext.ENGINE_SCOPE)
      mainEngine.eval(StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js"))

      val bios = new util.HashMap[String, Object]()
      executionThread = new NashornExecutionThread(machine, mainEngine, se => se.asInstanceOf[Invocable].invokeFunction("__bios__", bios))

      //Setup bios
      bios.put("bios", new JSBiosInternalAPI(machine, mainEngine))
      bios.put("component", new JSComponentApi(machine, executionThread.runSyncMethodCaller, connected.future))
      bios.put("computer", new JSComputerApi(machine, executionThread.signalHandler))

      //thread is started in runThreaded() after it's been connected to the network
      _initialized = true

      _initialized
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in initialize", e)
        machine.crash("Error in initialize: " + e.toString)
    }
  }

  override def recomputeMemory(components: Iterable[ItemStack]): Boolean = components.iterator().hasNext

  override def runSynchronized(): Unit = {
    try {
      if (executionThread.runSyncMethodCaller.outstandingSync) {
        //println("Sync call (synchronized)")
        executionThread.runSyncMethodCaller.executeSync()
      }
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in runSynchronized", e)
        machine.crash("Error in runSynchronized" + e.toString)
    }
  }

  override def close(): Unit = {
    //ScriptEngine can just be garbage collected
    sem = null
    mainEngine = null
    if (executionThread != null) executionThread.beNiceKill()
    executionThread = null

    _initialized = false
  }

  def kill(): Unit = {
  }

  override def save(nbt: NBTTagCompound): Unit = {
  }

  override def load(nbt: NBTTagCompound): Unit = {
    if (machine.isRunning) {
      machine.stop()
      machine.start()
    }
  }

  override def runThreaded(isSynchronizedReturn: Boolean): ExecutionResult = {
    try {
      if (!connected.isCompleted) {
        new ExecutionResult.Sleep(1)
      } else {
        if (!executionThread.started) {
          executionThread.start()
        }
        if (isSynchronizedReturn) {
          //println("Sync return (threaded)")
          //Trigger resume of javascript thread
          executionThread.runSyncMethodCaller.resolveSync()
          //Continue on our way
          new ExecutionResult.Sleep(1)
        } else {
          if (executionThread.runSyncMethodCaller.outstandingSync) {
            //println("Sync call (threaded)")
            new ExecutionResult.SynchronizedCall
          } else {
            if (!executionThread.signalHandler.push(machine.popSignal())) {
              executionThread.beNiceKill()
              return new ExecutionResult.Error("Javascript not responding; resorted to killing thread - this might be bad!")
            }
            new ExecutionResult.Sleep(1)
          }
        }
      }
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in runThreaded", e)
        new ExecutionResult.Error("Error in runThreaded: " + e.toString)
    }
  }
}
