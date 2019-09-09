package com.pwootage.oc.js

import li.cil.oc.api.machine.Signal
import java.util.*

class OCSignalHandler {
  val signalQue = ArrayDeque<Signal>()
  var lastPulled = System.currentTimeMillis()

  fun push(sig: Signal?): Boolean = synchronized(this) {
    if (signalQue.size > 128 || System.currentTimeMillis() - lastPulled > 10000) {
      false
    } else {
      if (sig != null) signalQue.offer(sig) else true
    }
  }

  fun pull(): Signal? = synchronized(this) {
    lastPulled = System.currentTimeMillis()
    signalQue.poll()
  }
}
