package com.pwootage.oc.js.api

import java.util

import com.pwootage.oc.js.{SyncMethodCaller, JSUtils}
import li.cil.oc.api.machine.{LimitReachedException, Machine}
import li.cil.oc.api.network.Component

import scala.collection.JavaConversions._
import scala.collection.JavaConverters._
import scala.concurrent._
import scala.concurrent.duration._

class JSComponentApi(machine: Machine, sync: SyncMethodCaller, connectedFuture: Future[Unit]) {
  def list(name: String): util.List[util.Map[String, String]] = connected { () =>
    machine.components.synchronized {
      machine.components().filter(t => t._2.contains(name)).map(t => Map(
        "uuid" -> t._1,
        "type" -> t._2
      ).asJava).toList.asJava
    }
  }

  def invoke(address: String, method: String, args: Array[AnyRef]): Array[AnyRef] = withComponent(address) { comp =>
    val m = machine.methods(comp.host).get(method)
    val invokeResult: Array[AnyRef] = if (m == null) {
      null
    } else {
      if (m.direct()) {
        try machine.invoke(address, method, args) catch {
          case e: LimitReachedException =>
            //Sync call
            sync.callSync(() => machine.invoke(address, method, args))
          case e: Throwable => throw e
        }
      } else {
        Await.result(sync.callSync(() => machine.invoke(address, method, args)), 10.seconds)
      }
    }
    JSUtils.scalaToJS(invokeResult)
  }

  def doc(address: String, method: String): String = withComponent(address) { comp =>
    Option(machine.methods(comp.host).get(method)).map(_.doc).orNull
  }

  def methods(address: String): util.Map[String, util.Map[String, Any]] = withComponent(address) { comp =>
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
