package com.pwootage.oc.js.v8

import java.util
import java.util.Optional

import com.pwootage.oc.js._
import com.pwootage.oc.js.jsvalue._
import li.cil.oc.api.machine.Signal

import scala.annotation.tailrec

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
    _started = true
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
    //    System.out.println("Eval thread", Thread.currentThread().getName)
    if (!filename.startsWith("__")) {
      println(s"Executing $filename")
    }
    val res = compile_and_execute(js, filename)
    val resVal = JSValue.fromJSON(res)

    if (resVal.property("state").asString.contains("error")) {
      println("Error executing: ", resVal)
    }

    resVal
  }

  override def executeThreaded(_signal: Optional[Signal], _syncResult: JSValue): RunThreadedResult = {
    @tailrec def execute(signal: Optional[Signal], syncResult: JSValue): RunThreadedResult = {
      val res = evalWithName("__test", "[...global.biosCallResultStore.entries()].toString()")
      val sigValue: JSValue = JSValue.fromJava(signal.orElse(null))
      val runJS = s"__biosRunThreaded(${sigValue.toJSON}, ${syncResult.toJSON});"
      val jsResRaw = evalWithName("__run_threaded", runJS)
      val jsState = jsResRaw.property("state").asString
      val jsRes = jsResRaw.property("result")
      if (!jsState.exists(_.equals("success"))) {
        throw new RuntimeException(s"Failed to execute: ${jsResRaw.toJSON}")
      }
      jsRes.property("type").asString match {
        case Some("sleep") =>
          RunThreadedResultSleep(
            (jsRes.property("arg").asDouble.getOrElse(0.05) * 20).toInt.max(1)
          )
        case Some("call") =>
          val call = jsRes.property("arg")
          val args = call.property("args")
          call.property("name").asString match {
            case Some("component.invoke") =>
              RunThreadedResultInvoke(
                call.property("id").asString.get,
                args.arrayVal(0).asString.getOrElse(""),
                args.arrayVal(1).asString.getOrElse(""),
                args.arrayVal(2).asArray.getOrElse(Array())
              )
            case _ =>
              val res = __call(call.toJSON)
              evalWithName("__biosReturn", s"__biosReturn($res);")
              execute(Optional.empty(), JSNull)
          }
        case Some("crash") =>
          throw new RuntimeException(s"JS crash: ${jsRes.property("arg").toJSON}")
        case _ =>
          throw new RuntimeException(s"Unknown yield type: ${jsRes.toJSON}")
      }
    }

    execute(_signal, _syncResult)
  }

  // Called from native
  protected def __call(callStr: String): String = {
    val call = JSValue.fromJSON(callStr)
    val fn = call.property("name").asString
    val id = call.property("id").asString
    val args = call.property("args")
    if (fn.isEmpty || id.isEmpty) {
      val res = new util.HashMap[String, String]()
      res.put("state", "error")
      res.put("value", "Must provide the name of the bios callback to call and the call ID")
      val resJson = JSValue.fromJava(res).toJSON
      println(s"Bad bios call: $call / ${resJson}")
      return resJson
    }

    val res = new util.HashMap[String, JSValue]()
    res.put("id", JSStringValue(id.get))
    val noop = JSStringValue("noop")
    val sync = JSStringValue("sync")
    val async = JSStringValue("async")
    val error = JSStringValue("error")
    fn.get match {
      case "bios.crash" =>
        println(s"Bios crash: ${args.arrayVal(0).asString}, ${args.arrayVal(1).asString}")
        arch.biosInternalAPI.crash(args.arrayVal(0).asString.getOrElse("Unspecified error"))
        res.put("state", noop)
      case "bios.log" =>
        arch.biosInternalAPI.log(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", noop)
      case "bios.compile" =>
        val compileRes = arch.biosInternalAPI.compile(
          args.arrayVal(0).asString.getOrElse("<anonymous.js>"),
          args.arrayVal(1).asString.getOrElse("")
        )
        if (compileRes.property("state").asString.contains("error")) {
          res.put("state", error)
          res.put("value", compileRes.property("value"))
        } else {
          res.put("state", sync)
          res.put("value", compileRes.property("value"))
        }
      case "component.list" =>
        val list = arch.componentAPI.list(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", sync)
        res.put("value", JSValue.fromJava(list))
      case "component.methods" =>
        val methods = arch.componentAPI.methods(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", sync)
        res.put("value", JSValue.fromJava(methods))
      case "component.invoke" =>
        res.put("state", async) // Must be executed from RunThreaded
      case "component.doc" =>
        val doc = arch.componentAPI.doc(
          args.arrayVal(0).asString.getOrElse(""),
          args.arrayVal(1).asString.getOrElse("")
        )
        res.put("state", sync)
        res.put("value", JSValue.fromJava(doc))
      case "component.type" =>
        val doc = arch.componentAPI.`type`(
          args.arrayVal(0).asString.getOrElse("")
        )
        res.put("state", sync)
        res.put("value", JSValue.fromJava(doc))
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