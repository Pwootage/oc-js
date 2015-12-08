package com.pwootage.oc.js

import java.io.{OutputStreamWriter, InputStreamReader}
import java.lang.Iterable
import javax.script.{Invocable, ScriptEngine}

import com.pwootage.oc.js.api.{JSComputerApi, JSBiosInternalAPI, JSComponentApi}
import jdk.nashorn.api.scripting.{ClassFilter, NashornScriptEngineFactory}
import li.cil.oc.api.machine.ExecutionResult.SynchronizedCall
import li.cil.oc.api.machine.{Architecture, ExecutionResult, Machine}
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound
import java.util

import scala.concurrent._
import scala.concurrent.duration._

/**
  * Nashorn Arch
  */
@Architecture.Name("ES5 (Nashorn)")
class NashornArchitecture(val machine: Machine) extends Architecture {
  private var _initialized = false

  private var sem: NashornScriptEngineFactory = null
  private var mainEngine: ScriptEngine = null
  private var executionThread: NashornExecutionThread = null

  override def isInitialized: Boolean = _initialized

  override def onConnect(): Unit = {
  }

  override def initialize(): Boolean = {
    try {
      if (_initialized) return _initialized

      sem = new NashornScriptEngineFactory()
      mainEngine = sem.getScriptEngine(Array("-strict", "--no-java", "--no-syntax-extensions"),
        Thread.currentThread().getContextClassLoader,
        new ClassFilter {
          override def exposeToScripts(s: String): Boolean = s.startsWith("com.pwootage.oc.nashorn.api")
        })
      //mainEngine.getContext.setWriter(new OutputStreamWriter(System.out))
      val kernelReader = new InputStreamReader(classOf[NashornArchitecture].getResourceAsStream("/assets/oc/js/bios/bios.js"))
      //Load kernel
      mainEngine.eval(kernelReader)

      val bios = new util.HashMap[String, Object]()
      executionThread = new NashornExecutionThread(machine, mainEngine, se => se.asInstanceOf[Invocable].invokeFunction("__bios__", bios))

      //Setup bios
      bios.put("bios", new JSBiosInternalAPI(machine, mainEngine))
      bios.put("component", new JSComponentApi(machine, executionThread.runSyncMethodCaller))
      bios.put("computer", new JSComputerApi(machine, executionThread.signalHandler))

      executionThread.start()

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
        println("Sync call (synchronized)")
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
      if (isSynchronizedReturn) {
        println("Sync return (threaded)")
        //Trigger resume of javascript thread
        executionThread.runSyncMethodCaller.resolveSync()
        //Continue on our way
        new ExecutionResult.Sleep(1)
      } else {
        if (executionThread.runSyncMethodCaller.outstandingSync) {
          println("Sync call (threaded)")
          new ExecutionResult.SynchronizedCall
        } else {
          if (!executionThread.signalHandler.push(machine.popSignal())) {
            executionThread.beNiceKill()
            return new ExecutionResult.Error("Javascript not responding; resorted to killing thread - this might be bad!")
          }
          new ExecutionResult.Sleep(1)
        }
      }
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in runThreaded", e)
        new ExecutionResult.Error("Error in runThreaded: " + e.toString)
    }
  }
}
