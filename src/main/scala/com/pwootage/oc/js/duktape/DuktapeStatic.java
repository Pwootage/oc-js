package com.pwootage.oc.js.duktape;

public class DuktapeStatic {
  private static boolean initialized = false;

  public static boolean isInitialized() {
    return initialized;
  }

  public static synchronized void initialize() {
    if (initialized) {
      return;
    }
    System.out.println("Initializing Duktape");
    initialized = true;
    native_init();
    System.out.println("SpiderMonkey Duktape");
  }

  native private static void native_init();
}
