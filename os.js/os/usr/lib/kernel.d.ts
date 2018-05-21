// Do nothing but cause this to actually compile
// ¯\_(ツ)_/¯
import {} from 'usr/lib/kernel';

declare global {
  const require: (file: string) => any;

  const exports: any;
  const module: ModuleInfo;

  interface ModuleInfo {
    name:string;
    path:string;
  }
}
