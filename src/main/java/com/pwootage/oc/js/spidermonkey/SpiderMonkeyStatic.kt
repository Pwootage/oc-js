package com.pwootage.oc.js.spidermonkey

import com.google.common.io.ByteStreams

import java.io.IOException
import java.io.InputStream
import java.nio.ByteBuffer

object SpiderMonkeyStatic {
    var isInitialized = false
        private set

    //  public static String getPlatform() {
    //    String os = System.getProperty("os.name").toLowerCase();
    //    String arch = System.getProperty("os.arch").toLowerCase();
    //    if (!(arch.contains("x") && arch.contains("64"))) {
    //      throw new Error("OC-JS only supports x86_64 for now");
    //    }
    //    if (os.contains("win")) {
    //      return "windows.x64";
    //    } else if (os.contains("mac")) {
    //      return "macos.x64";
    //    } else {
    //      // Sorry, anyone that's not one of the Big Three, I guess
    //      return "linux.x64";
    //    }
    //  }

    @Synchronized
    fun initialize() {
        if (isInitialized) {
            return
        }
        println("Initializing SpiderMonkey")
        isInitialized = true
        native_init()
        println("SpiderMonkey Initialized")
    }

    private external fun native_init()
}
