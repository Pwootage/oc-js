package com.pwootage.oc.js

import net.minecraftforge.fml.common.Mod
import net.minecraftforge.fml.common.Mod.EventHandler
import net.minecraftforge.fml.common.event.{FMLPostInitializationEvent, FMLInitializationEvent, FMLPreInitializationEvent}
import org.apache.logging.log4j.LogManager

@Mod(
  modid = OCJS.ID,
  name = OCJS.Name,
  version = OCJS.Version,
  modLanguage = "scala",
  useMetadata = true,
  dependencies = "required-after:OpenComputers@[1.5.20,)"
)
object OCJS {
  final val ID = "oc.js"

  final val Name = "oc-js"

  final val Version = "@VERSION@"

  var log = LogManager.getLogger("OC-JS")

  @EventHandler
  def preInit(e: FMLPreInitializationEvent): Unit = {
    li.cil.oc.api.Machine.add(classOf[NashornArchitecture])
  }

  @EventHandler
  def init(e: FMLInitializationEvent): Unit = {
  }

  @EventHandler
  def postInit(e: FMLPostInitializationEvent): Unit = {
  }
}
