package com.pwootage.oc.js.api

import java.util
import javax.script.ScriptEngine

import com.pwootage.oc.js.{NashornArchitecture, OCJSApi}
import jdk.nashorn.api.scripting.ScriptObjectMirror

import scala.collection.JavaConversions._

class JSComponentApi(arch: NashornArchitecture) extends OCJSApi(arch) {
  class ComponentAPI {
    /*
    component.doc
    component.fields
    component.invoke
    component.list
    component.methods
    component.proxy
    component.slot
    component.type
     */
    def list(name: String): util.Map[String, String] = {
      arch.machine.components.synchronized {
        arch.machine.components().filter(t => t._2.contains(name))
      }
    }

    def invoke(address:String, method:String, args:AnyRef*): AnyRef = {
      arch.machine.invoke(address, method, args.toArray)
    }

    def apply():Int = 5
  }


  override def register(engine: ScriptEngine): Unit = {
    engine.put("component", new ComponentAPI)
  }

}
