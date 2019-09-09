package com.pwootage.oc.js

import java.io.InputStreamReader

import com.google.common.io.CharStreams

object StaticJSSrc {
  fun loadSrc(path: String): String {
    return InputStreamReader(OCJS::class.java.getResourceAsStream(path)).use {
      it.readText()
    }
  }
}
