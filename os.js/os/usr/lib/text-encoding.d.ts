// Type definitions for text-encoding
// Project: https://github.com/inexorabletash/text-encoding
// Definitions by: Pine Mizune <https://github.com/pine>
//                 Mohsen Azimi <https://github.com/mohsen1>
//                 Thomas Nicollet <https://github.com/nwmqpa>
// Definitions: https://github.com/DefinitelyTyped/DefinitelyTyped
// TypeScript Version: 2.8
// modified a bunch to work properly here

export {}
declare global {
  var TextEncoder: {
    new(utfLabel?: string): TextEncoder;
    (utfLabel?: string): TextEncoder;
    encoding: string;
  };

  var TextDecoder: {
    (label?: string): TextDecoder;
    new(label?: string): TextDecoder;
    encoding: string;
  };

  interface TextEncodeOptions {
    stream?: boolean;
  }

  interface TextDecoderOptions {
    stream?: boolean;
  }

  interface TextEncoder {
    readonly encoding: string;
    encode(input?: string,ions?: TextEncodeOptions): Uint8Array;
  }

  interface TextDecoder {
    readonly encoding: string;
    decode(input?: Uint8Array, options?: TextDecoderOptions): string;
  }
}
