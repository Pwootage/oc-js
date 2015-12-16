package com.pwootage.oc.js

import java.io.InputStreamReader

import com.google.common.io.CharStreams

object StaticJSSrc {
  def loadSrc(path:String): String = {
    val in = new InputStreamReader(classOf[NashornArchitecture].getResourceAsStream(path))
    val res = CharStreams.toString(in)
    in.close()
    res
  }
}
