package com.pwootage.oc.js.api

import com.pwootage.oc.js.JSEngine
import com.pwootage.oc.js.OCJS

import li.cil.oc.api.machine.Machine

class JSBiosInternalAPI(val machine: Machine, val scriptEngine: JSEngine) {
  fun log(msg: String): Unit {
    // TODO: no-op this (or disable it)
    OCJS.log.error(msg)
  }

  fun crash(msg: String): Unit {
    machine.crash(msg)
    throw JSExitException("JS Crashed: $msg")
  }
}
