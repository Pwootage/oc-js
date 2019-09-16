package com.pwootage.oc.js

abstract class JSEngine(private val arch: JSArchitectureBase) {
  abstract val started: Boolean
  abstract fun start()
  abstract fun destroy()
  abstract fun setMaxMemory(max: Int): Unit
  abstract val maxMemory: Int
  abstract val usedMemory: Int

  abstract fun jsPushNext(jsNext: JSValue): JSValue

  fun executeThreaded(jsNext: JSValue): RunThreadedResult {
    tailrec fun execute(jsNextRecursive: JSValue): RunThreadedResult {
      val jsYield = jsPushNext(jsNextRecursive)
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
      "computer.address" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.address())
      }
      "computer.tmpAddress" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.tmpAddress())
      }
      "computer.freeMemory" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.freeMemory())
      }
      "computer.totalMemory" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.totalMemory())
      }
      "computer.energy" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.energy())
      }
      "computer.maxEnergy" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.maxEnergy())
      }
      "computer.uptime" -> {
        res["state"] = success
        res["value"] = JSValue.fromJava(arch.computerAPI.uptime())
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
}
