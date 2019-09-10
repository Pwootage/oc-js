package com.pwootage.oc.js.duktape

import com.pwootage.oc.js.*

class DuktapeEngine(val arch: DuktapeArchitecture) : JSEngine {
  private var duktapeEngineNative: Long = 0
  private var _maxMemory: Int = 0
  private var allocatedMemory: Int = 0
  private var _started = false

  // Initialize on startup
  init {
    if (!DuktapeStatic.isInitialized) {
      DuktapeStatic.initialize()
    }
    native_start()
  }

  override fun setMaxMemory(max: Int) {
    _maxMemory = max
  }

  override val maxMemory get() = _maxMemory

  override val usedMemory get() = allocatedMemory

  override val started get() = _started

  override fun start() {
    _started = true
    val biosJS = StaticJSSrc.loadSrc("/assets/oc-js/bios/bios.js")
    val shim = StaticJSSrc.loadSrc("/assets/oc-js/es6-shim.min.js")
    val biosSrc =
      """$shim;(function(exports, global){try {$biosJS
           return "<bios ended execution>";
         /**/} catch (error) {
           const backup = new Error('<no error somehow>');
           return error.stack || backup.stack || '<no stack>';
         }
         })({}, this);
        """
    arch.componentInvoker.setResult(JSStringValue(biosSrc))
  }

  override fun destroy() {
    native_destroy()
  }

  override fun executeThreaded(jsNext: JSValue): RunThreadedResult {
    tailrec fun execute(jsNextRecursive: JSValue): RunThreadedResult {
      val jsYield = native_next(jsNextRecursive)
      return when (jsYield.property("type").asString) {
        "__bios__" -> RunThreadedResultSleep(0)
        "sleep" -> RunThreadedResultSleep(
          ((jsYield.property("duration").asDouble ?: 0.05) * 20).toInt().coerceIn(0..20)
        )
        "call" -> {
          val args = jsYield.property("args")
          when (jsYield.property("name").asString) {
            // Handle component invoke here
            "component.invoke" -> {
              RunThreadedResultInvoke(
                args.arrayVal(0).asString ?: "",
                args.arrayVal(1).asString ?: "",
                args.arrayVal(2).asArray ?: emptyArray()
              )
            }
            else -> execute(execCall(jsYield))
          }
        }
        "crash", "error" -> {
          var message = "JS crash: "
          if (jsYield.property("file").asString != null) {
            message += jsYield.property("file").asString
          }
          if (jsYield.property("line").asDouble != null) {
            message += ":" + jsYield.property("line").asDouble?.toLong()
          }
          if (jsYield.property("start").asDouble != null) {
            message += ":" + jsYield.property("start").asDouble?.toLong()
          }
          message += " " + (jsYield.property("message").asString ?: "<unknown error>")
          throw RuntimeException(message)
        }
        "exec_end" ->
          throw RuntimeException("JS crash: " + jsYield.property("result").asString)
        "unresponsive" ->
          throw RuntimeException("JS unresponsive")
        else ->
          throw RuntimeException("Unknown yield type: $jsYield/${jsYield.toJSON()}")
      }
    }

    return execute(jsNext)
  }

  private fun execCall(call: JSValue): JSValue {
    val fn = call.property("name").asString
    val args = call.property("args")
    if (fn.isNullOrBlank()) {
      val res = JSMap(mapOf(
        "state" to JSStringValue("error"),
        "value" to JSStringValue("Must provide the name of the bios callback to call and the call ID")
      ))
      println("Bad bios call: $call / ${res.toJSON()}")
      return res
    }

    val res = HashMap<String, JSValue>()
    val error = JSStringValue("error")
    val success = JSStringValue("success")
    when (fn) {
      "bios.crash" -> {
        println("Bios crash: ${args.arrayVal(0).asString}, ${args.arrayVal(1).asString}")
        arch.biosInternalAPI.crash(args.arrayVal(0).asString ?: "Unspecified error")
        // Doesn't return, it throws
        res["state"] = error
      }
      "bios.log" -> {
        arch.biosInternalAPI.log(args.arrayVal(0).asString ?: "")
        res["state"] = success
      }
      "component.list" -> {
        val list = arch.componentAPI.list(args.arrayVal(0).asString ?: "")
        res["state"] = success
        res["value"] = JSValue.fromJava(list)
      }
      "component.methods" -> {
        val methods = arch.componentAPI.methods(args.arrayVal(0).asString ?: "")
        res["state"] = success
        res["value"] = JSValue.fromJava(methods)
      }
      "component.doc" -> {
        val doc = arch.componentAPI.doc(
          args.arrayVal(0).asString ?: "",
          args.arrayVal(1).asString ?: ""
        )
        res["state"] = success
        res["value"] = JSValue.fromJava(doc)
      }
      "component.type" -> {
        val doc = arch.componentAPI.type(
          args.arrayVal(0).asString ?: ""
        )
        res["state"] = success
        res["value"] = JSValue.fromJava(doc)
      }
      else -> {
        res["state"] = error
        res["value"] = JSStringValue("Unknown bios call: $fn")
        val resJson = JSMap(res).toJSON()
        println("Unknown bios call: $fn / $resJson")
      }
    }
    return JSMap(res)
  }

  //Native methods
  private external fun native_start(): Unit

  private external fun native_destroy(): Unit

  private external fun native_next(next: JSValue): JSValue
}