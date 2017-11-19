package com.pwootage.oc.js.v8

import java.nio.ByteBuffer
import java.util

import com.google.common.io.ByteStreams
import com.pwootage.oc.js.{JSArchitectureBase, JSEngine}
import com.pwootage.oc.js.jsvalue.{JSMap, JSNull, JSStringValue, JSValue}

import scala.collection.mutable

class V8Engine(arch: V8Architecture) extends JSEngine {
  private var v8EngineNative: Long = 0
  private var _maxMemory: Int = 0
  private var allocatedMemory: Int = 0
  private var _started = false

  // Initialize on startup
  if (!V8Static.isInitialized) {
    V8Static.initialize()
  }
  native_start()

  override def setMaxMemory(max: Int): Unit = _maxMemory = max

  override def maxMemory: Int = _maxMemory

  override def usedMemory: Int = allocatedMemory

  override def started: Boolean = _started

  override def start(): Unit = {
    val res = evalWithName("__bios_main", "__bios_main().then(() => {})")
//    val res = evalWithName("__bios_main", "__bios_call({name: 'test', args: []})")
    if (res.property("state").asString.exists(_.equals("error"))) {
      println("Error starting bios: " + res.toJSON)
      throw new RuntimeException("Failed to start: " + res.property("message"))
    } else {
      println("Bios result: " + res.toJSON)
    }
  }

  override def destroy(): Unit = {
    native_destroy()
  }

  override def evalWithName(filename: String, js: String): JSValue = {
    System.out.println("Eval thread", Thread.currentThread().getName)
    val res = compile_and_execute(js, filename)
    JSValue.fromJSON(res)
  }

  override def executeThreaded(syncResult: JSValue): JSValue = {
    println(s"NEED TO EXECUTE THREADED")
    JSNull
  }

  // Called from native
  private def __call(callStr: String): String = {
    val call = JSValue.fromJSON(callStr)
    val fn = call.property("name").asString
    val args = call.property("args")
    if (fn.isEmpty) {
      val res = new util.HashMap[String, String]()
      res.put("state", "error")
      res.put("value", "Must provide the name of the bios callback to call")
      val resJson = JSValue.fromJava(res).toJSON
      println(s"Bad bios call: $call / ${resJson}")
      return resJson
    }

    val res = new util.HashMap[String, JSValue]()
    val noop = JSStringValue("noop")
    val sync = JSStringValue("sync")
    val error = JSStringValue("error")
    fn.get match {
      case "bios.crash" =>
        println(s"Bios crash: ${args.arrayVal(0).asString}")
        arch.biosInternalAPI.crash(args.arrayVal(0).asString.getOrElse("Unspecified error"))
        res.put("state", noop)
      case "component.list" =>
        val list = arch.componentAPI.list(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", sync)
        res.put("value", JSValue.fromJava(list))
      case x =>
        res.put("state", error)
        res.put("value", JSStringValue(s"Unknown bios call: $x"))
        val resJson = JSMap(res).toJSON
        println(s"Unknown bios call: x / $resJson")
    }
    JSMap(res).toJSON
  }

  //Native methods
  @native protected def native_start(): Unit

  @native protected def native_destroy(): Unit

  @native protected def compile_and_execute(src: String, filename: String): String
}