package com.pwootage.oc.js

import java.lang.Iterable
import java.util

import com.pwootage.oc.js.api.{JSBiosInternalAPI, JSComponentApi, JSComputerApi}
import com.pwootage.oc.js.jsvalue.{JSArray, JSUndefined, JSValue}
import li.cil.oc.api.Driver
import li.cil.oc.api.driver.item.Memory
import li.cil.oc.api.machine.{Architecture, ExecutionResult, LimitReachedException, Machine}
import li.cil.oc.api.network.Component
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound

import scala.annotation.tailrec
import scala.concurrent._
import scala.concurrent.duration._
import scala.collection.convert.WrapAsScala._

trait InvokeResult

case class InvokeResultComplete(res: Array[AnyRef]) extends InvokeResult

case class InvokeResultSyncCall(call: () => Array[AnyRef]) extends InvokeResult

/**
  * JS base
  */
abstract class JSArchitectureBase(val machine: Machine) extends Architecture {
  private var _initialized = false
  private var _started = false
  private var connectedPromise: Promise[Unit] = Promise()

  private var mainEngine: JSEngine = null
  val signalHandler = new OCSignalHandler
  private val componentInvoker = new ComponentInvoker

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
    connectedPromise.synchronized {
      if (!connectedPromise.isCompleted) connectedPromise.success(Unit)
    }
  }

  override def onSignal(): Unit = {
  }

  override def initialize(): Boolean = {
    try {
      if (_initialized) return _initialized
      //Check if it's already connected (onConnect doesn't get called, otherwise)
      connectedPromise.synchronized {
        if (!connectedPromise.isCompleted && machine.node() != null && machine.node().network() != null) connectedPromise.success(Unit)
      }

      mainEngine = createEngine()
      setupSandbox()

      //Load bios
      val biosJS = StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js")
      mainEngine.evalWithName("bios.js", biosJS)

      val bios = new util.HashMap[String, Object]()

      //Setup bios
      bios.put("bios", new JSBiosInternalAPI(machine, mainEngine))
      bios.put("component", new JSComponentApi(machine, connectedPromise.future))
      bios.put("computer", new JSComputerApi(machine, signalHandler, mainEngine))

      //thread is started in runThreaded() after it's been connected to the network
      _initialized = true
      _initialized
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in initialize", e)
        machine.crash("Error in initialize: " + e.toString)
    }
  }

  override def recomputeMemory(components: java.lang.Iterable[ItemStack]): Boolean = {
    val memory = math.ceil(memoryInBytes(components) * 1.8).toInt
    //TODO: allow unlimited memory?
    if (_initialized) {
      mainEngine.setMaxMemory(memory)
    }
    memory > 0
  }

  private def memoryInBytes(components: java.lang.Iterable[ItemStack]) = components.foldLeft(0.0)((acc, stack) => acc + (Option(Driver.driverFor(stack)) match {
    case Some(driver: Memory) => driver.amount(stack) * 1024
    case _ => 0
  })).toInt max 0 min 64 * 1024 * 1024 // TODO: 64mb? Sure! Make configurable.

  override def runSynchronized(): Unit = {
    try {
      if (componentInvoker.hasSyncCall) {
        println("Sync call (synchronized)")
        componentInvoker.executeSync()
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
    @tailrec def executeThreaded(): ExecutionResult = {
      val jsRunResult = mainEngine.executeThreaded(componentInvoker.result().getOrElse(JSUndefined))
      val yieldType = jsRunResult.property("type").asString.getOrElse(throw new Exception("RunThreaded did not yield!"))
      yieldType match {
        case "sleep" =>
          val sleepAmount = jsRunResult.property("arg").asDouble
            .getOrElse(1.0)
            .toInt
          new ExecutionResult.Sleep(sleepAmount)
        case "invoke" =>
          val address = jsRunResult.property("arg").property("address").asString.getOrElse("<no address passed>")
          val method = jsRunResult.property("arg").property("method").asString.getOrElse("<no method passed>")
          val args = jsRunResult.property("arg").property("args").asArray.getOrElse(Array())
          invoke(address, method, args) match {
            case x: InvokeResultComplete =>
              //We yeilded successfully, we can just run again
              componentInvoker.setResult(JSValue.javaToJsValue(x.res))
              executeThreaded()
            case x: InvokeResultSyncCall =>
              componentInvoker.callSync(() => JSValue.javaToJsValue(x.call))
              new ExecutionResult.SynchronizedCall
          }
      }
    }

    try {
      if (!connectedPromise.isCompleted) {
        new ExecutionResult.Sleep(1)
      } else {
        if (!mainEngine.started) {
          mainEngine.start()
        }
        if (!isSynchronizedReturn) {
          println("Sync return (threaded)")
          if (!signalHandler.push(machine.popSignal())) {
            mainEngine.destroy()
            return new ExecutionResult.Error("Javascript not responding; resorted to killing engine!")
          }
        }
        executeThreaded()
      }
    } catch {
      case e: Throwable =>
        OCJS.log.error("Error in runThreaded", e)
        new ExecutionResult.Error("Error in runThreaded: " + e.toString)
    }
  }

  private def invoke(address: String, method: String, jsArgs: Array[JSValue]): InvokeResult = withComponent(address) { comp =>
    val m = machine.methods(comp.host).get(method)
    if (m == null) {
      return InvokeResultComplete(Array())
    } else {
      val args = jsArgs.flatMap(_.asSimpleJava)
      if (m.direct()) {
        val res = try machine.invoke(address, method, args) catch {
          case e: LimitReachedException =>
            //Sync call
            return InvokeResultSyncCall(() => machine.invoke(address, method, args))
          case e: Throwable => throw e
        }
        return InvokeResultComplete(res)
      } else {
        return InvokeResultSyncCall(() => machine.invoke(address, method, args))
      }
    }
  }

  private def withComponent[T](address: String)(f: (Component) => T): T = whenConnected { () =>
    Option(machine.node.network.node(address)) match {
      case Some(component: Component) if component.canBeSeenFrom(machine.node) || component == machine.node =>
        f(component)
      case _ =>
        null.asInstanceOf[T] //eew
    }
  }

  private def whenConnected[T](fn: () => T) = {
    if (!connectedPromise.isCompleted) Await.result(connectedPromise.future, 10.seconds)
    fn()
  }
}
