package com.pwootage.oc.js.api

import com.pwootage.oc.js.OCJS
import li.cil.oc.api.machine.Machine

class JSBiosDebugApi(machine: Machine) {
  def log(msg: String): Unit = {
    OCJS.log.error(msg)
  }

  def crash(msg: String): Unit = {
    machine.crash(msg)
    throw new JSExitException("JS Crashed: " + msg)
  }
}
