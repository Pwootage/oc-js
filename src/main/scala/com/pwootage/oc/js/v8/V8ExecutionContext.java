package com.pwootage.oc.js.v8;

public enum V8ExecutionContext {
  BIOS(0),
  KERNEL(1),
  USER(2);

  private V8ExecutionContext(int raw) {
    this.raw = raw;
  }

  public final int raw;
}
