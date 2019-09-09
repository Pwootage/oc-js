package com.pwootage.oc.js.api

import li.cil.oc.api.machine.Machine
import li.cil.oc.api.network.Component
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit

class JSComponentApi(val machine: Machine, private val connectedFuture: CompletableFuture<Unit>) {
  fun list(name: String): List<Map<String, String>> = connected {
    synchronized(machine.components()) {
      machine.components()
        .filter { it.value.contains(name) }
        .map {
          mapOf(
            "uuid" to it.key,
            "type" to it.value
          )
        }
        .toList()
    }
  }

  fun doc(address: String, method: String): String? = withComponent(address) { comp ->
    machine.methods(comp.host())[method]?.doc
  }

  fun methods(address: String): Map<String, Map<String, Any>> = withComponent(address) {
    val methodsRes = mutableMapOf<String, Map<String, Any>>()

    machine.methods(it.host()).forEach { (name, callback) ->
      methodsRes[name] = mapOf<String, Any>(
        "name" to name,
        "doc" to callback.doc,
        "direct" to callback.direct,
        "limit" to callback.limit,
        "getter" to callback.getter,
        "setter" to callback.setter
      )
    }

    methodsRes
  }

  fun type(address: String): String? = connected {
    synchronized(machine.components()) {
      machine.components()[address]
    }
  }

  private fun <T> withComponent(address: String, f: (Component) -> T): T = connected {
    val component = machine.node().network().node(address) as? Component

    if (component != null && (component.canBeReachedFrom(machine.node()) || component == machine.node())) {
      f(component)
    } else {
      // TODO: is this ok? or does this need to be nullable?
      throw IllegalStateException("Couldn't find component")
    }
  }

  private fun <T> connected(fn: () -> T): T {
    if (!connectedFuture.isDone) connectedFuture.get(10, TimeUnit.SECONDS)
    return fn()
  }
}
