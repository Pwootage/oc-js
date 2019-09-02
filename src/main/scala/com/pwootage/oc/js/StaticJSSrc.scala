package com.pwootage.oc.js

import java.io.InputStreamReader

import com.google.common.io.CharStreams
import com.pwootage.oc.js.spidermonkey.SpiderMonkeyArchitecture

object StaticJSSrc {
  def loadSrc(path: String): String = {
    val in = new InputStreamReader(classOf[SpiderMonkeyArchitecture].getResourceAsStream(path))
    val res = CharStreams.toString(in)
    in.close()
    res
  }
}
