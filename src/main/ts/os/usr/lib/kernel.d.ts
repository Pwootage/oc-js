// Do nothing but cause this to actually compile
// ¯\_(ツ)_/¯
import {} from '';

declare global {
  const require: (file: string) => Promise<any>;

  const exports: any;
  const module: ModuleInfo;

  interface ModuleInfo {
    name:string;
    path:string;
  }
}
