package com.pwootage.oc.js

import java.util

import li.cil.oc.api.machine.Signal

class OCSignalHandler {
  val signalQue = new util.ArrayDeque[Signal]()
  var lastPulled = System.currentTimeMillis()

  def push(sig: Signal): Boolean = this.synchronized {
    if (signalQue.size() > 128 || System.currentTimeMillis() - lastPulled > 10000) {
      false
    } else {
      if (sig != null) signalQue.offer(sig) else true
    }
  }

  def pull(): Option[Signal] = this.synchronized {
    lastPulled = System.currentTimeMillis()
    Option(signalQue.poll())
  }
}
