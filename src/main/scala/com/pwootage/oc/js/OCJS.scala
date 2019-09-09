package com.pwootage.oc.js

import java.lang.management.ManagementFactory
import java.nio.file.Paths
import java.util.concurrent.Callable
import java.util.{Timer, TimerTask}

import com.google.common.io.ByteStreams
import com.pwootage.oc.js.duktape.{DuktapeArchitecture, DuktapeStatic}
import com.pwootage.oc.js.spidermonkey.{SpiderMonkeyArchitecture, SpiderMonkeyStatic}
import li.cil.oc.api._
import net.minecraft.item.EnumDyeColor
import net.minecraftforge.fml.common.Mod
import net.minecraftforge.fml.common.Mod.EventHandler
import net.minecraftforge.fml.common.event.{FMLInitializationEvent, FMLPostInitializationEvent, FMLPreInitializationEvent}
import org.apache.logging.log4j.LogManager

//@Mod(
//  modid = com.pwootage.oc.js.OCJS.ID,
//  name = com.pwootage.oc.js.OCJS.Name,
//  version = com.pwootage.oc.js.OCJS.Version,
//  modLanguage = "scala",
//  useMetadata = true,
//  dependencies = "required-after:opencomputers@[1.7.2,)"
//)
//object com.pwootage.oc.js.OCJS {
//  final val ID = "oc-js"
//
//  final val Name = "oc-js"
//
//  final val Version = "@VERSION@"
//
//  var log = LogManager.getLogger("OC-JS")
//
//  @EventHandler
//  def preInit(e: FMLPreInitializationEvent): Unit = {
//    //Pre-load nashorn to save time later
//    //    log.info("Pre-loading Nashorn...")
//    //    new NashornScriptEngineFactory().getScriptEngine.eval("1 + 1")
//    //    log.info("Pre-loaded Nashorn.")
//
//    val pid = ManagementFactory.getRuntimeMXBean().getName()
//    log.info(pid)
//    log.info("Loading duktape natives")
//    //TODO: make this work in not, well, dev
//    //        System.load(Paths.get("../native/cmake-build-debug/libocjs.so").toAbsolutePath.normalize().toString)
//    try {
//      //      System.load(Paths.get("../native/cmake-build-debug/libmozglue.dylib").toAbsolutePath.normalize().toString)
//      //      System.load(Paths.get("../native/cmake-build-debug/libmozjs-70a1.dylib").toAbsolutePath.normalize().toString)
//      System.load(Paths.get("../native/cmake-build-debug/libocjs.dylib").toAbsolutePath.normalize().toString)
//    } catch {
//      case e: Throwable =>
//        e.printStackTrace()
//        throw e;
//    }
//    SpiderMonkeyStatic.initialize()
//    DuktapeStatic.initialize()
//    log.info("Loaded ocjs natives")
//
//    Machine.add(classOf[DuktapeArchitecture])
//    Machine.add(classOf[SpiderMonkeyArchitecture])
//
//    val is = classOf[SpiderMonkeyArchitecture].getResourceAsStream("/assets/oc-js/bios/bootloader.js")
//    Items.registerEEPROM("EEPROM (jsboot)", ByteStreams.toByteArray(is), null, true)
//    is.close()
//
//    Items.registerFloppy("oc.js", EnumDyeColor.LIGHT_BLUE, new Callable[fs.FileSystem] {
//      override def call(): fs.FileSystem = FileSystem.fromClass(classOf[SpiderMonkeyArchitecture], "oc-js", "os/")
//    }, true)
//
//    val t = new Timer()
//    val task = new TimerTask {
//      override def run() = {
//        log.info(pid)
//      }
//    }
//    //        t.scheduleAtFixedRate(task, 2000, 2000)
//  }
//
//  @EventHandler
//  def init(e: FMLInitializationEvent): Unit = {
//  }
//
//  @EventHandler
//  def postInit(e: FMLPostInitializationEvent): Unit = {
//  }
//}
