package com.pwootage.oc.js.spidermonkey;

import com.google.common.io.ByteStreams;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class SpiderMonkeyStatic {
  private static boolean initialized = false;

  public static boolean isInitialized() {
    return initialized;
  }

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

  public static synchronized void initialize() {
    if (initialized) {
      return;
    }
    System.out.println("Initializing SpiderMonkey");
    initialized = true;
    native_init();
    System.out.println("SpiderMonkey Initialized");
  }

  native private static void native_init();
}
