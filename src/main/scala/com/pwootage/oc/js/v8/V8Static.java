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

  static boolean isInitialized() {
    return initialized;
  }

  static synchronized void initialize() {
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
    icudtl = loadBytes("/assets/oc-js/v8/icudtl.dat");
    natives_blob = loadBytes("/assets/oc-js/v8/natives_blob.bin");
    snapshot_blob = loadBytes("/assets/oc-js/v8/snapshot_blob.bin");
  }

  private static ByteBuffer loadBytes(String path) throws IOException {
    InputStream in = V8Architecture.class.getResourceAsStream(path);
    byte[] bytes = ByteStreams.toByteArray(in);
    in.close();
    ByteBuffer res = ByteBuffer.allocateDirect(bytes.length);
    res.put(bytes);
    return res;
  }

  native private static void native_init();
}
