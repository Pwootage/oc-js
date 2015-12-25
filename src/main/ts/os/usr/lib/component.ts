///ts:ref=kernel.d.ts
/// <reference path="../kernel/kernel.d.ts"/> ///ts:ref:generated
///ts:ref=bios.d.ts
/// <reference path="../../../bios/bios.d.ts"/> ///ts:ref:generated

class ComponentAPI {
  constructor() {
  }

  first(type:string):any {
    return $bios.component.first(type);
  }

  all(type:string):any[] {
    return $bios.component.list(type).map(v => $bios.component.proxy(v.uuid));
  }
}


export = new ComponentAPI();
