package com.pwootage.oc.js.api

import java.util

import com.pwootage.oc.js.NashornArchitecture
import li.cil.oc.api.machine.Machine
import li.cil.oc.api.network.Component

import scala.collection.JavaConversions._
import scala.collection.JavaConverters._

class JSComponentApi(machine: Machine) {
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
  def list(name: String): util.Map[String, String] = machine.components.synchronized {
    machine.components().filter(t => t._2.contains(name))
  }

  def invoke(address: String, method: String, args: Array[AnyRef]): AnyRef = {
    machine.invoke(address, method, args)
  }

  def doc(address: String, method: String): String = withComponent(address) { comp =>
    Option(machine.methods(comp.host).get(method)).map(_.doc).orNull
  }

  def methods(address: String): util.Map[String,  util.Map[String, Any]] = withComponent(address) { comp =>
    machine.methods(comp.host).map(t => {
      val (name, callback) = t
      (name, Map[String, Any](
        "name" -> name,
        "doc" -> callback.doc(),
        "direct" -> callback.direct(),
        "limit" -> callback.limit(),
        "getter" -> callback.getter(),
        "setter" -> callback.setter()
      ).asJava)
    }).toMap.asJava
  }

  def `type`(address: String): String = machine.components.synchronized {
    machine.components().get(address)
  }

  private def withComponent[T](address: String)(f: (Component) => T): T = Option(machine.node.network.node(address)) match {
    case Some(component: Component) if component.canBeSeenFrom(machine.node) || component == machine.node =>
      f(component)
    case _ =>
      null.asInstanceOf[T] //eew
  }
}
