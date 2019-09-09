package com.pwootage.oc.js


import com.pwootage.oc.js.api.JSBiosInternalAPI
import com.pwootage.oc.js.api.JSComponentApi
import com.pwootage.oc.js.api.JSComputerApi
import li.cil.oc.api.Driver
import li.cil.oc.api.driver.item.Memory
import li.cil.oc.api.machine.Architecture
import li.cil.oc.api.machine.ExecutionResult
import li.cil.oc.api.machine.LimitReachedException
import li.cil.oc.api.machine.Machine
import li.cil.oc.api.network.Component
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import kotlin.math.ceil
import kotlin.math.roundToInt

sealed class InvokeResult

data class InvokeResultComplete(val res: Array<Any>?) : InvokeResult()
data class InvokeResultSyncCall(val call: () -> Array<Any>?) : InvokeResult()

sealed class RunThreadedResult
data class RunThreadedResultSleep(val time: Int) : RunThreadedResult()
data class RunThreadedResultInvoke(val address: String, val method: String, val args: Array<JSValue>) : RunThreadedResult()

/**
 * JS base
 */
abstract class JSArchitectureBase(val machine: Machine) : Architecture {
  private var _initialized = false
  private val connectedPromise = CompletableFuture<Unit>()

  private var mainEngine: JSEngine? = null
  val componentInvoker = ComponentInvoker()
  lateinit var biosInternalAPI: JSBiosInternalAPI
  lateinit var componentAPI: JSComponentApi
  lateinit var computerAPI: JSComputerApi

  // Methods for subclesses

  /**
   * Creates the JSEngine for this architecture - sandbox setup happens in a later step, just create the engine here
   */
  abstract fun createEngine(): JSEngine

  //Architecture methods

  override fun isInitialized(): Boolean = _initialized

  override fun onConnect() {
    synchronized(connectedPromise) {
      if (!connectedPromise.isDone) connectedPromise.complete(Unit)
    }
  }

  override fun onSignal() {
    // TODO?
  }

  override fun initialize(): Boolean {
    try {
      if (_initialized) return _initialized
      //Check if it's already connected (onConnect doesn't get called, otherwise)
      synchronized(connectedPromise) {
        if (!connectedPromise.isDone && machine.node() != null && machine.node().network() != null) connectedPromise.complete(Unit)
      }

      mainEngine = createEngine()

      //Bios to be loaded by impl
      //Set up api wrappers
      biosInternalAPI = JSBiosInternalAPI(machine, mainEngine!!)
      componentAPI = JSComponentApi(machine, connectedPromise)
      computerAPI = JSComputerApi(machine, mainEngine!!)

      //thread is started in runThreaded() after it's been connected to the network
      _initialized = true
    } catch (e: Throwable) {
      OCJS.log.error("Error in initialize", e)
      machine.crash("Error in initialize: $e")
    }
    return _initialized
  }

  override fun recomputeMemory(components: Iterable<ItemStack>): Boolean {
    // TODO: config option for 64 bit ratio? Do we need this ratio?
    val memory = ceil(memoryInBytes(components) * 1.8).toInt()
    //TODO: allow unlimited memory?
    if (_initialized) {
      mainEngine?.setMaxMemory(memory)
    }
    return memory > 0
  }

  private fun memoryInBytes(components: Iterable<ItemStack>) = components.sumByDouble {
    when (val mem = Driver.driverFor(it)) {
      is Memory -> mem.amount(it) * 1024
      else -> 0.0
    }
  }.roundToInt().coerceIn(0..64 * 1024 * 1024) // TODO: 64mb? Sure! Make configurable.

  override fun runSynchronized() {
    try {
      if (componentInvoker.hasSyncCall) {
//        println("Sync call (synchronized)")
        componentInvoker.executeSync()
      }
    } catch (e: Throwable) {
      OCJS.log.error("Error in runSynchronized", e)
      machine.crash("Error in runSynchronized: $e")
    }
  }

  override fun close() {
    mainEngine?.destroy()
    mainEngine = null
    _initialized = false
  }

  override fun save(nbt: NBTTagCompound) {
  }

  override fun load(nbt: NBTTagCompound) {
    if (machine.isRunning) {
      machine.stop()
      machine.start()
    }
  }

  override fun runThreaded(isSynchronizedReturn: Boolean): ExecutionResult {
    tailrec fun executeThreaded(signal: JSValue?): ExecutionResult {
      val invokeResult = componentInvoker.result()
      if (signal != null && invokeResult != null) {
        throw RuntimeException("Both a signal and an invoke result exist! This is not supposed to be possible!")
      }
      val next = invokeResult ?: signal ?: JSNull
      return when (val jsRunResult = mainEngine!!.executeThreaded(next)) {
        is RunThreadedResultSleep ->
          ExecutionResult.Sleep(jsRunResult.time)

        is RunThreadedResultInvoke -> when (val invokeRes = invoke(jsRunResult.address, jsRunResult.method, jsRunResult.args)) {
          is InvokeResultComplete -> {
            //We yeilded successfully, we can just run again
            componentInvoker.setResult(JSMap(mapOf(
              "state" to JSStringValue("success"),
              "value" to JSValue.fromJava(invokeRes.res)
            )))
            executeThreaded(null)
          }
          is InvokeResultSyncCall -> {
            componentInvoker.callSync {
              val callResult = invokeRes.call()
              val jsvalueResult = JSValue.fromJava(callResult)
              JSMap(mapOf(
                "state" to JSStringValue("success"),
                "value" to jsvalueResult
              ))
            }
            ExecutionResult.SynchronizedCall()
          }
        }
      }
    }

    return try {
      if (!connectedPromise.isDone) {
        ExecutionResult.Sleep(1)
      } else {
        if (!mainEngine!!.started) {
          mainEngine!!.start()
        }
        var signal: JSValue? = null
        if (!isSynchronizedReturn) {
          val sig = machine.popSignal()
          if (sig != null) {
            signal = JSMap(mapOf(
              "state" to JSStringValue("success"),
              "value" to JSValue.fromJava(sig)
            ))
          }
        }
        executeThreaded(signal)
      }
    } catch (e: Throwable) {
      OCJS.log.error("Error in runThreaded", e)
      ExecutionResult.Error("Error in runThreaded: $e")
    }
  }

  protected fun invoke(address: String, method: String, jsArgs: Array<JSValue>): InvokeResult = withComponent(address) { comp ->
    val m = machine.methods(comp.host())[method]
    if (m == null) {
      InvokeResultComplete(emptyArray())
    } else {
      val args = jsArgs.map { it.asSimpleJava }.toTypedArray()
      if (m.direct) {
        val res = try {
          machine.invoke(address, method, args)
        } catch (e: LimitReachedException) {
          return@withComponent InvokeResultSyncCall { machine.invoke(address, method, args) }
        }
        InvokeResultComplete(res)
      } else {
        InvokeResultSyncCall { machine.invoke(address, method, args) }
      }
    }
  }


  private fun <T> withComponent(address: String, f: (Component) -> T): T = connected {
    val component = machine.node().network().node(address) as? Component

    if (component != null && (component.canBeReachedFrom(machine.node()) || component == machine.node())) {
      f(component)
    } else {
      // TODO: is this ok? or does this need to be nullable?
      throw IllegalStateException("Couldn't find component")
    }
  }

  private fun <T> connected(fn: () -> T): T {
    if (!connectedPromise.isDone) connectedPromise.get(10, TimeUnit.SECONDS)
    return fn()
  }
}
