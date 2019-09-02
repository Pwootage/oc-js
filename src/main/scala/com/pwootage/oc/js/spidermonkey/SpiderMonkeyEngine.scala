package com.pwootage.oc.js.spidermonkey

import java.util
import java.util.Optional

import com.pwootage.oc.js._
import com.pwootage.oc.js.jsvalue._

import scala.annotation.tailrec

class SpiderMonkeyEngine(arch: SpiderMonkeyArchitecture) extends JSEngine {
  private var spiderMonkeyEngineNative: Long = 0
  private var _maxMemory: Int = 0
  private var allocatedMemory: Int = 0
  private var _started = false

  // Initialize on startup
  if (!SpiderMonkeyStatic.isInitialized) {
    SpiderMonkeyStatic.initialize()
  }
  native_start()

  override def setMaxMemory(max: Int): Unit = _maxMemory = max

  override def maxMemory: Int = _maxMemory

  override def usedMemory: Int = allocatedMemory

  override def started: Boolean = _started

  override def start(): Unit = {
    _started = true
    val biosJS = StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js")
    val biosSrc =
      s"""(function(exports, global){try {$biosJS
         |  return "<bios ended execution>";
         |/**/} catch (error) {
         |  const backup = new Error('<no error somehow>');
         |  return error.stack || backup.stack || '<no stack>';
         |}
         |})({}, this);
        """.stripMargin
    arch.componentInvoker.setResult(JSStringValue(biosSrc))
  }

  override def destroy(): Unit = {
    native_destroy()
  }

  override def executeThreaded(_jsNext: JSValue): RunThreadedResult = {
    @tailrec def execute(jsNext: JSValue): RunThreadedResult = {
      val jsNextStr = jsNext match {
        case JSStringValue(x) => x
        case x=> x.toJSON
      }
      val nativeRes = native_next(jsNextStr)
      val jsYield = try {
        JSValue.fromJSON(nativeRes)
      } catch {
        case e: Throwable =>
          println("Bad native res: " + nativeRes)
          e.printStackTrace()
          throw e
      }
      jsYield.property("type").asString match {
        case Some("__bios__") =>
          RunThreadedResultSleep(0)
        case Some("sleep") =>
          RunThreadedResultSleep(
            (jsYield.property("duration").asDouble.getOrElse(0.05) * 20).toInt.max(1)
          )
        case Some("call") =>
          val call = jsYield
          val args = call.property("args")
          call.property("name").asString match {
            // Handle component invoke here
            case Some("component.invoke") =>
              RunThreadedResultInvoke(
                args.arrayVal(0).asString.getOrElse(""),
                args.arrayVal(1).asString.getOrElse(""),
                args.arrayVal(2).asArray.getOrElse(Array())
              )
            case _ =>
              execute(execCall(call))
          }
        case Some("crash") | Some("error") =>
          var message = "JS crash: "
          if (jsYield.property("file").asString.isDefined) {
            message += jsYield.property("file").asString.get
          }
          if (jsYield.property("line").asDouble.isDefined) {
            message += ":" + jsYield.property("line").asDouble.get.toLong
          }
          if (jsYield.property("start").asDouble.isDefined) {
            message += ":" + jsYield.property("start").asDouble.get.toLong
          }
          message += " " +  jsYield.property("message").asString.getOrElse("<unknown error>")
          throw new RuntimeException(message)
        case Some("exec_end") =>
          throw new RuntimeException("JS crash: " + jsYield.property("result").asString)
        case Some("unresponsive") =>
          throw new RuntimeException(s"JS unresponsive")
        case x =>
          throw new RuntimeException(s"Unknown yield type: $x/${jsYield.toJSON}")
      }
    }

    execute(_jsNext)
  }

  protected def execCall(call: JSValue): JSValue = {
    val fn = call.property("name").asString
    val args = call.property("args")
    if (fn.isEmpty) {
      val res = JSMap(
        "state" -> JSStringValue("error"),
        "value" -> JSStringValue("Must provide the name of the bios callback to call and the call ID")
      )
      println(s"Bad bios call: $call / ${res.toJSON}")
      return res
    }

    val res = new util.HashMap[String, JSValue]()
    val error = JSStringValue("error")
    val success = JSStringValue("success")
    fn.get match {
      case "bios.crash" =>
        println(s"Bios crash: ${args.arrayVal(0).asString}, ${args.arrayVal(1).asString}")
        arch.biosInternalAPI.crash(args.arrayVal(0).asString.getOrElse("Unspecified error"))
        // Doesn't return, it throws
        res.put("state", error)
      case "bios.log" =>
        arch.biosInternalAPI.log(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", success)
      case "component.list" =>
        val list = arch.componentAPI.list(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", success)
        res.put("value", JSValue.fromJava(list))
      case "component.methods" =>
        val methods = arch.componentAPI.methods(args.arrayVal(0).asString.getOrElse(""))
        res.put("state", success)
        res.put("value", JSValue.fromJava(methods))
      case "component.doc" =>
        val doc = arch.componentAPI.doc(
          args.arrayVal(0).asString.getOrElse(""),
          args.arrayVal(1).asString.getOrElse("")
        )
        res.put("state", success)
        res.put("value", JSValue.fromJava(doc))
      case "component.type" =>
        val doc = arch.componentAPI.`type`(
          args.arrayVal(0).asString.getOrElse("")
        )
        res.put("state", success)
        res.put("value", JSValue.fromJava(doc))
      case x =>
        res.put("state", error)
        res.put("value", JSStringValue(s"Unknown bios call: $x"))
        val resJson = JSMap(res).toJSON
        println(s"Unknown bios call: x / $resJson")
    }
    JSMap(res)
  }

  //Native methods
  @native protected def native_start(): Unit

  @native protected def native_destroy(): Unit

  @native protected def native_next(next: String): String
}