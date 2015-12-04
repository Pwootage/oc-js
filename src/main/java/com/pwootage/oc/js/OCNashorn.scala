package com.pwootage.oc.js

import net.minecraftforge.fml.common.Mod
import net.minecraftforge.fml.common.Mod.EventHandler
import net.minecraftforge.fml.common.event.{FMLPostInitializationEvent, FMLInitializationEvent, FMLPreInitializationEvent}
import org.apache.logging.log4j.LogManager

@Mod(
  modid = OCNashorn.ID,
  name = OCNashorn.Name,
  version = OCNashorn.Version,
  modLanguage = "scala",
  useMetadata = true,
  dependencies = "required-after:OpenComputers@[1.5.20,)"
)
object OCNashorn {
  final val ID = "oc.nashorn"

  final val Name = "oc-nashorn"

  final val Version = "@VERSION@"

  var log = LogManager.getLogger("OC-QEMU")

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
