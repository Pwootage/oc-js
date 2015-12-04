package com.pwootage.oc.js

import java.io.{OutputStreamWriter, InputStreamReader}
import java.lang.Iterable
import javax.script.ScriptEngine

import com.pwootage.oc.js.api.{JSConsoleDebugApi, JSComponentApi}
import jdk.nashorn.api.scripting.{ClassFilter, NashornScriptEngineFactory}
import li.cil.oc.api.machine.{Architecture, ExecutionResult, Machine}
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound

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

    //Register APIs
    new JSComponentApi(this).register(mainEngine)
    new JSConsoleDebugApi(this).register(mainEngine)

    val kernelReader = new InputStreamReader(classOf[NashornArchitecture].getResourceAsStream("/assets/oc-nashorn/bios/bios.js"))
    executionThread = new NashornExecutionThread(mainEngine, kernelReader)
    executionThread.start()

    _initialized = true

    _initialized
  }

  override def recomputeMemory(components: Iterable[ItemStack]): Boolean = components.iterator().hasNext

  override def runSynchronized(): Unit = {
    try {

    } catch {
      case e: Throwable => OCNashorn.log.error("Unknown exception was thrown by CPU!", e)
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
      case e: Throwable => OCNashorn.log.warn("Error in CPU loop: ", e)
    }
    new ExecutionResult.Sleep(1)
  }
}
