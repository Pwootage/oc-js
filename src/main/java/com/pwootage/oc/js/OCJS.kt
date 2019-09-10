package com.pwootage.oc.js

import com.pwootage.oc.js.duktape.DuktapeArchitecture
import com.pwootage.oc.js.duktape.DuktapeStatic
import li.cil.oc.api.Items
import li.cil.oc.api.Machine
import li.cil.oc.api.FileSystem
import net.minecraft.item.EnumDyeColor
import net.minecraftforge.fml.common.Mod
import net.minecraftforge.fml.common.event.FMLInitializationEvent
import net.minecraftforge.fml.common.event.FMLPostInitializationEvent
import net.minecraftforge.fml.common.event.FMLPreInitializationEvent
import org.apache.logging.log4j.LogManager
import java.lang.management.ManagementFactory
import java.nio.file.Paths
import java.util.concurrent.Callable

@Mod(
  modid = OCJS.ID,
  name = OCJS.Name,
  version = OCJS.Version,
  modLanguage = "kotlin",
  modLanguageAdapter = "net.shadowfacts.forgelin.KotlinAdapter",
  useMetadata = true,
  dependencies = "required-after:opencomputers@[1.7.2,);required-after:forgelin@[1.8,);"
)
object OCJS {
  const val ID = "oc-js"
  const val Name = "oc-js"
  const val Version = "@VERSION@"
  val log = LogManager.getLogger("OC-JS")

  @Mod.EventHandler
  fun preInit(e: FMLPreInitializationEvent) {
    //Pre-load nashorn to save time later
    //    log.info("Pre-loading Nashorn...")
    //    new NashornScriptEngineFactory().getScriptEngine.eval("1 + 1")
    //    log.info("Pre-loaded Nashorn.")

    val pid = ManagementFactory.getRuntimeMXBean().getName()
    log.info(pid)
    log.info("Loading duktape natives")
    //TODO: make this work in not, well, dev
    //        System.load(Paths.get("../native/cmake-build-debug/libocjs.so").toAbsolutePath.normalize().toString)
    try {
      //      System.load(Paths.get("../native/cmake-build-debug/libmozglue.dylib").toAbsolutePath.normalize().toString)
      //      System.load(Paths.get("../native/cmake-build-debug/libmozjs-70a1.dylib").toAbsolutePath.normalize().toString)
      System.load(Paths.get("../native/cmake-build-debug/libocjs.dylib").toAbsolutePath().normalize().toString())
    } catch (e: Throwable) {
      e.printStackTrace()
      throw e
    }
//    SpiderMonkeyStatic.initialize()
    DuktapeStatic.initialize()
    log.info("Loaded ocjs natives")

    Machine.add(DuktapeArchitecture::class.java)
//    Machine.add(SpiderMonkeyArchitecture::class.java)

    Items.registerEEPROM("EEPROM (jsboot)", StaticJSSrc.loadSrc("/assets/oc-js/bios/bootloader.js").toByteArray(), null, true)

    Items.registerFloppy("oc.js", EnumDyeColor.LIGHT_BLUE, {
      FileSystem.fromClass(OCJS::class.java, "oc-js", "os/")
    }, true)

//    val t = Timer()
//    val task = object : TimerTask {
//      override fun run() {
//        log.info(pid)
//      }
//    }
    //        t.scheduleAtFixedRate(task, 2000, 2000)
  }

  @Mod.EventHandler
  fun init(e: FMLInitializationEvent) {
  }

  @Mod.EventHandler
  fun postInit(e: FMLPostInitializationEvent) {
  }
}