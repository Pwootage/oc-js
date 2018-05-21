export abstract class OCComponent {
  constructor(public readonly id: string, public readonly type: string) {

  }

  invoke(method: string, args: any[]): any {
    throw new Error("No such method")
  }
}