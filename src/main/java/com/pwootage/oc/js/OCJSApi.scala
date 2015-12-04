package com.pwootage.oc.js

import javax.script.ScriptEngine

import jdk.nashorn.api.scripting.ScriptObjectMirror

abstract class OCJSApi(arch: NashornArchitecture) {
  def register(engine: ScriptEngine): Unit
}
