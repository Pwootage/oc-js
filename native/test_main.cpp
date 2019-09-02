#include <thread>

using namespace std;

#include <jsapi.h>
#include <js/Initialization.h>
#include <js/Conversions.h>
#include <js/JSON.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>


using namespace JS;

// global class ops
static JSClassOps global_ops = {
  nullptr, //addProperty
  nullptr, //delProperty
  nullptr, //enumerate
  nullptr, //newEnumerate
  nullptr, //resolve
  nullptr, //mayResolve
  nullptr, //finalize
  nullptr, //call
  nullptr, //hasInstance
  nullptr, //construct
  JS_GlobalObjectTraceHook
};

/* The class of the global object. */
static JSClass global_class = {
  "global",
  JSCLASS_GLOBAL_FLAGS,
  &global_ops
};

void reportError(JSContext *cx, const char *message, JSErrorReport *report) {
  fprintf(stderr, "%s:%u:%s\n",
          report->filename ? report->filename : "[no filename]",
          (unsigned int) report->lineno,
          message);
}

u16string getU16String(JSContext *ctx, const JS::HandleValue &handle) {
  JS::RootedString str(ctx, JS::ToString(ctx, handle));
  if (!str) return u"";// TODO: throw error?
  size_t len = JS_GetStringLength(str);
  if (len > 1024*1024) return u"";// TODO: throw error

  u16string res;
  res.resize(len);
  JS_CopyStringChars(ctx, mozilla::Range<char16_t>(res.data(), res.length()), str);
  return res;
}

bool __yield(JSContext *ctx, unsigned argc, JS::Value *vp) {
  // Grab the engine
//  auto *native = static_cast<SpiderMonkeyEngineNative *>(JS_GetContextPrivate(ctx));

  printf("Debug JSGC_BYTES: %d\n", JS_GetGCParameter(ctx, JSGC_BYTES));
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // pull out args
  u16string json = getU16String(ctx, args[0]);

  // yield
  u16string res = json;

  // Return
  args.rval().setString(JS_NewUCStringCopyN(ctx, res.c_str(), res.length()));
  return true;
}

int main(int argc, const char *argv[])
{
  JS_Init();

  JSContext *ctx = JS_NewContext(8L * 1024 * 1024);
  if (!ctx) return 1;
  if (!InitSelfHostedCode(ctx)) return 2;

  printf("Debug JSGC_BYTES: %d\n", JS_GetGCParameter(ctx, JSGC_BYTES));
  JS_SetGCParameter(ctx, JSGC_MAX_BYTES, 700000);
  JS_SetGCParameter(ctx, JSGC_MAX_MALLOC_BYTES, 700000);
  JS_SetNativeStackQuota(ctx, 256 * 1024);
  JS_SetGCParametersBasedOnAvailableMemory(ctx, 700000);

  {
    RealmOptions options;
    RootedObject global(ctx, JS_NewGlobalObject(ctx, &global_class, nullptr, FireOnNewGlobalHook, options));
    if (!global) return 3;


    printf("Debug JSGC_BYTES: %d\n", JS_GetGCParameter(ctx, JSGC_BYTES));
    RootedValue rval(ctx);

    { // Scope for JSAutoRealm
      JSAutoRealm ar(ctx, global);
      if (!InitRealmStandardClasses(ctx)) return 4;
      if (!JS_DefineFunction(ctx, global, "__yield", __yield, 1, 0)) return 7;


      const char16_t *script = uR"('hello '+' world, it is '+new Date()+':'+ __yield('asdf')


    const huger = [];
    let i = 0;
    try {
      for (;i<1024; i++) {
        const arr = new Uint32Array(1024*1024 / 4);
        arr.fill(i);
        huger.push(arr);
        __yield(i);
      }
    } catch (e) {
      throw e;
    }
)";
      const size_t script_len = char_traits<char16_t>::length(script);


      const char *filename = "noname";
      int lineno = 1;
      CompileOptions opts(ctx);
      opts.setFileAndLine(filename, lineno);
      SourceText<char16_t> srcBuf;
      if (!srcBuf.init(ctx, script, script_len, SourceOwnership::Borrowed)) return 5;
      if (!Evaluate(ctx, opts, srcBuf, &rval)) return 6;
    }

    RootedString str(ctx, rval.toString());
    UniqueChars strChars = JS_EncodeStringToUTF8(ctx, str);
    printf("%s\n", strChars.get());
    fflush(stdout);
  }

  JS_DestroyContext(ctx);
  JS_ShutDown();
  return 0;
}

