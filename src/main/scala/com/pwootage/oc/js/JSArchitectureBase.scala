package com.pwootage.oc.js

import java.lang.Iterable
import java.util
import javax.script.{Invocable, ScriptContext, ScriptEngine}

import com.pwootage.oc.js.api.{JSBiosInternalAPI, JSComponentApi, JSComputerApi}
import jdk.nashorn.api.scripting.{ClassFilter, NashornScriptEngineFactory}
import li.cil.oc.api.machine.{Architecture, ExecutionResult, Machine}
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound

import scala.concurrent._

/**
  * JS base
  */
abstract class JSArchitectureBase(val machine: Machine) extends Architecture {
  private var _initialized = false
  private var _started = false
  private var connected: Promise[Unit] = Promise()

  private var mainEngine: JSEngine = null
  val signalHandler = new OCSignalHandler
  private val syncMethodCaller = new SyncMethodCaller

  // Methods for subclesses

  /**
    * Creates the JSEngine for this architecture - sandbox setup happens in a later step, just create the engine here
    */
  def createEngine(): JSEngine

  /**
    * Do any sandbox setup here
    */
  def setupSandbox(): Unit

  //Architecture methods

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
      //Check if it's already connected (onConnect doesn't get called, otherwise)
      connected.synchronized {
        if (!connected.isCompleted && machine.node() != null && machine.node().network() != null) connected.success(Unit)
      }

      mainEngine = createEngine()
      setupSandbox()

      //Load bios
      val biosJS = StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js")
      mainEngine.evalWithName("bios.js", biosJS)

      val bios = new util.HashMap[String, Object]()

      //Setup bios
      bios.put("bios", new JSBiosInternalAPI(machine, mainEngine))
      bios.put("component", new JSComponentApi(machine, syncMethodCaller, connected.future))
      bios.put("computer", new JSComputerApi(machine, signalHandler))

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
      if (syncMethodCaller.hasSyncCall) {
        println("Sync call (synchronized)")
        syncMethodCaller.executeSync()
      }
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in runSynchronized", e)
        machine.crash("Error in runSynchronized" + e.toString)
    }
  }

  override def close(): Unit = {
    mainEngine.destroy()
    mainEngine = null
    _initialized = false
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
        if (!mainEngine.started) {
          mainEngine.start()
        }
        if (isSynchronizedReturn) {
          println("Sync return (threaded)")
          mainEngine.executeThreaded(syncMethodCaller.result())
          new ExecutionResult.Sleep(1)
        } else {
          if (syncMethodCaller.hasSyncCall) {
            println("Sync call (threaded)")
            new ExecutionResult.SynchronizedCall
          } else {
            if (!signalHandler.push(machine.popSignal())) {
              mainEngine.destroy()
              return new ExecutionResult.Error("Javascript not responding; resorted to killing engine!")
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
