package com.pwootage.oc.js

import java.nio.file.Paths
import java.util.concurrent.Callable

import com.google.common.io.ByteStreams
import com.pwootage.oc.js.duktape.DuktapeArchitecture
import jdk.nashorn.api.scripting.NashornScriptEngineFactory
import li.cil.oc.api._
import net.minecraft.item.EnumDyeColor
import net.minecraftforge.fml.common.Mod
import net.minecraftforge.fml.common.Mod.EventHandler
import net.minecraftforge.fml.common.event.{FMLInitializationEvent, FMLPostInitializationEvent, FMLPreInitializationEvent}
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
  final val ID = "oc-js"

  final val Name = "oc-js"

  final val Version = "@VERSION@"

  var log = LogManager.getLogger("OC-JS")

  @EventHandler
  def preInit(e: FMLPreInitializationEvent): Unit = {
    //Pre-load nashorn to save time later
//    log.info("Pre-loading Nashorn...")
//    new NashornScriptEngineFactory().getScriptEngine.eval("1 + 1")
//    log.info("Pre-loaded Nashorn.")

    log.info("Loading duktape natives")
    //TODO: make this work in not, well, dev
    System.load(Paths.get("../native/cmake-build-debug/libocjs.so").toAbsolutePath.normalize().toString)
    log.info("Loaded duktape natives")

//    Machine.add(classOf[NashornArchitecture])
    Machine.add(classOf[DuktapeArchitecture])

    val is = classOf[DuktapeArchitecture].getResourceAsStream("/assets/oc-js/bios/bootloader.js")
    Items.registerEEPROM("EEPROM (jsboot)", ByteStreams.toByteArray(is), null, true)
    is.close()

    Items.registerFloppy("oc.js", EnumDyeColor.LIGHT_BLUE, new Callable[fs.FileSystem] {
      override def call(): fs.FileSystem = FileSystem.fromClass(classOf[DuktapeArchitecture], "oc-js", "os/")
    })
  }

  @EventHandler
  def init(e: FMLInitializationEvent): Unit = {
  }

  @EventHandler
  def postInit(e: FMLPostInitializationEvent): Unit = {
  }
}
