package com.pwootage.oc.js.v8;

import com.google.common.io.ByteStreams;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class V8Static {
  private static boolean initialized = false;
  private static ByteBuffer icudtl;
  private static ByteBuffer natives_blob;
  private static ByteBuffer snapshot_blob;

  public static boolean isInitialized() {
    return initialized;
  }

  public static String getPlatform() {
    String os = System.getProperty("os.name").toLowerCase();
    String arch = System.getProperty("os.arch").toLowerCase();
    if (!(arch.contains("x") && arch.contains("64"))) {
      throw new Error("OC-JS only supports x86_64 for now");
    }
    if (os.contains("win")) {
      return "windows.x64";
    } else if (os.contains("mac")) {
      return "macos.x64";
    } else {
      // Sorry, anyone that's not one of the Big Three, I guess
      return "linux.x64";
    }
  }

  public static synchronized void initialize() {
    if (initialized) {
      return;
    }
    System.out.println("Initializing V8");
    initialized = true;
    try {
      loadBlobs();
    } catch (IOException e) {
      throw new RuntimeException("Can't init V8", e);
    }
    native_init();
    System.out.println("V8 Initialized");
  }

  private static void loadBlobs() throws IOException {
    String platform = getPlatform();
    icudtl = loadBytes("/assets/oc-js/v8/" + platform + "/icudtl.dat");
    natives_blob = loadBytes("/assets/oc-js/v8/" + platform + "/natives_blob.bin");
    snapshot_blob = loadBytes("/assets/oc-js/v8/" + platform + "/snapshot_blob.bin");
  }

  private static ByteBuffer loadBytes(String path) throws IOException {
    InputStream in = V8Architecture.class.getResourceAsStream(path);
    byte[] bytes = ByteStreams.toByteArray(in);
    in.close();
    ByteBuffer res = ByteBuffer.allocateDirect(bytes.length);
    res.put(bytes);
    res.rewind();
    return res;
  }

  native private static void native_init();
}
