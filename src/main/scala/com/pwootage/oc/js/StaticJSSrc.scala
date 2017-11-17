package com.pwootage.oc.js

import java.io.InputStreamReader

import com.google.common.io.CharStreams
import com.pwootage.oc.js.v8.V8Architecture

object StaticJSSrc {
  def loadSrc(path: String): String = {
    val in = new InputStreamReader(classOf[V8Architecture].getResourceAsStream(path))
    val res = CharStreams.toString(in)
    in.close()
    res
  }
}
