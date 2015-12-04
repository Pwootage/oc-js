package com.pwootage.oc.js.api

import java.util
import javax.script.ScriptEngine

import com.pwootage.oc.js.{NashornArchitecture, OCJSApi}

import scala.collection.JavaConversions._

class JSConsoleDebugApi(arch: NashornArchitecture) extends OCJSApi(arch) {
  class ConsoleAPI {
    def log(msg:String): Unit = {
      println(msg)
    }
  }


  override def register(engine: ScriptEngine): Unit = {
    engine.put("console", new ConsoleAPI)
  }

}
