package com.pwootage.oc.js.api

import java.util

import com.pwootage.oc.js.{ComponentInvoker, JSUtils}
import li.cil.oc.api.machine.{LimitReachedException, Machine}
import li.cil.oc.api.network.Component

import scala.collection.JavaConversions._
import scala.collection.JavaConverters._
import scala.concurrent._
import scala.concurrent.duration._

class JSComponentApi(machine: Machine, connectedFuture: Future[Unit]) {
  def list(name: String): util.List[util.Map[String, String]] = connected { () =>
    machine.components.synchronized {
      val resList = new util.ArrayList[util.Map[String, String]]()
      machine.components()
        .filter(t => t._2.contains(name))
        .map(t => {
          val resMap = new util.HashMap[String, String]()
          resMap.put("uuid", t._1)
          resMap.put("type", t._2)
          resMap
        })
        .foreach(resList.add(_))
      resList
    }
  }

  def doc(address: String, method: String): String = withComponent(address) { comp =>
    Option(machine.methods(comp.host).get(method)).map(_.doc).orNull
  }

  def methods(address: String): util.Map[String, util.Map[String, Any]] = withComponent(address) { comp =>
    val methodsRes = new util.HashMap[String, util.Map[String,Any]]()

    machine.methods(comp.host).foreach(t => {
      val (name, callback) = t
      val methodRes = new util.HashMap[String, Any]()
      methodRes.put("name", name)
      methodRes.put("doc", callback.doc())
      methodRes.put("direct", callback.direct())
      methodRes.put("limit", callback.limit())
      methodRes.put("getter", callback.getter())
      methodRes.put("setter", callback.setter())
      methodsRes.put(name, methodRes);
    })

    methodsRes
  }

  def invoke(address: String, method: String, args: Array[AnyRef]): Array[AnyRef] = withComponent(address) { comp =>
    comp.invoke(method, machine, args:_*)
  }

  def `type`(address: String): String = connected { () =>
    machine.components.synchronized {
      machine.components().get(address)
    }
  }

  private def withComponent[T](address: String)(f: (Component) => T): T = connected { () =>
    Option(machine.node.network.node(address)) match {
      case Some(component: Component) if component.canBeSeenFrom(machine.node) || component == machine.node =>
        f(component)
      case _ =>
        null.asInstanceOf[T] //eew
    }
  }

  private def connected[T](fn: () => T) = {
    if (!connectedFuture.isCompleted) Await.result(connectedFuture, 10.seconds)
    fn()
  }
}
