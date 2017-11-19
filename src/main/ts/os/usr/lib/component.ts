class ComponentAPI {
  constructor() {
  }

  first(type:string): Promise<any> {
    return $bios.component.first(type);
  }

  async all(type:string): Promise<any[]> {
    return (await $bios.component.list(type))
              .map(v => $bios.component.proxy(v.uuid));
  }
}

export const component = new ComponentAPI();
export default component;
