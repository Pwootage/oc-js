package com.pwootage.oc.js

import java.io.{OutputStreamWriter, InputStreamReader}
import java.lang.Iterable
import javax.script.{Invocable, ScriptEngine}

import com.pwootage.oc.js.api.JSBiosDebugApi
import jdk.nashorn.api.scripting.{ClassFilter, NashornScriptEngineFactory}
import com.pwootage.oc.js.api.JSComponentApi
import li.cil.oc.api.machine.{Architecture, ExecutionResult, Machine}
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound
import java.util

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

    //Setup bios
    val bios = new util.HashMap[String, Object]()
    bios.put("component", new JSComponentApi(machine))
    bios.put("console", new JSBiosDebugApi(machine))

    executionThread = new NashornExecutionThread(machine, mainEngine, se => se.asInstanceOf[Invocable].invokeFunction("__bios__", bios))
    executionThread.start()

    _initialized = true

    _initialized
  }

  override def recomputeMemory(components: Iterable[ItemStack]): Boolean = components.iterator().hasNext

  override def runSynchronized(): Unit = {
    try {

    } catch {
      case e: Throwable => OCJS.log.error("Unknown exception was thrown by CPU!", e)
    }
  }

  override def close(): Unit = {
    //ScriptEngine can just be garbage collected
    sem = null
    mainEngine = null

    _initialized = false
  }

  def kill(): Unit = {
  }

  override def save(nbt: NBTTagCompound): Unit = {
  }

  override def load(nbt: NBTTagCompound): Unit = {
  }

  override def runThreaded(isSynchronizedReturn: Boolean): ExecutionResult = {
    try {

    } catch {
      case e: Throwable => OCJS.log.warn("Error in CPU loop: ", e)
    }
    new ExecutionResult.Sleep(1)
  }
}
