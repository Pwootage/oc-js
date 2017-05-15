package com.pwootage.oc.js.duktape

import java.lang.Iterable
import java.util
import javax.script.{Invocable, ScriptContext, ScriptEngine}

import com.pwootage.oc.js.api.{JSBiosInternalAPI, JSComponentApi, JSComputerApi}
import com.pwootage.oc.js._
import jdk.nashorn.api.scripting.{ClassFilter, NashornScriptEngineFactory}
import li.cil.oc.api.machine.{Architecture, ExecutionResult, Machine}
import net.minecraft.item.ItemStack
import net.minecraft.nbt.NBTTagCompound

import scala.concurrent._

/**
  * Nashorn Arch
  */
@Architecture.Name("ES5 (duktape)")
class DuktapeArchitecture(_machine: Machine) extends JSArchitectureBase(_machine) {
  override def createEngine(): JSEngine = {
    new DuktapeEngine()
  }

  override def setupSandbox(): Unit = {
    //TODO
  }
}
