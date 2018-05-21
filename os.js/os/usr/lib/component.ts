class ComponentAPI {
  constructor() {
  }

  first(type:string): any {
    return $bios.component.first(type);
  }

  all(type:string): any[] {
    return $bios.component.list(type).map(v => $bios.component.proxy(v.uuid));
  }
}

export const component = new ComponentAPI();
export default component;
