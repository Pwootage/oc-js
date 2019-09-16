//
// Created by pwootage on 11/14/17.
//
#include "SpiderMonkeyEngineNative.h"
#include <js/Conversions.h>
#include <js/JSON.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/ArrayBuffer.h>
#include <js/MemoryFunctions.h>
#include <jsfriendapi.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <functional>
#include <locale>
#include <codecvt>

using namespace std;

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

SpiderMonkeyEngineNative::SpiderMonkeyEngineNative() {
  this->mainThread = thread([this] { this->mainThreadFn(); });
}

SpiderMonkeyEngineNative::~SpiderMonkeyEngineNative() {
  debug_print(u"JS main thread kill");
  shouldKill = true;
  if (!isDead) {
    debug_print(u"JS main thread is not dead");
    JS_RequestInterruptCallback(this->context);
    this->next(OCJS::JSValuePtr(new OCJS::JSNullValue));
    JS_RequestInterruptCallback(this->context);
  }
  debug_print(u"JS waiting for main thread");
  this->mainThread.join();
  debug_print(u"JS main thread kill complete");
}

future<OCJS::JSValuePtr> SpiderMonkeyEngineNative::next(OCJS::JSValuePtr next) {
  future<OCJS::JSValuePtr> res;
  {
    lock_guard<mutex> lock(this->executionMutex);

    if (this->deadResult) {
      promise<OCJS::JSValuePtr> promise;
      promise.set_value(*this->deadResult);
      return promise.get_future();
    }

    this->nextInput = make_optional(next);
    this->outputPromise = make_optional(promise<OCJS::JSValuePtr>());
    res = this->outputPromise->get_future();
  }
  this->engineWait.notify_one();
  return res;
}

void SpiderMonkeyEngineNative::mainThreadFn() {
  // Lock the engine
  this->engineLock = unique_lock(this->executionMutex);

  debug_print(u"JS main thread start");

  //init
  this->context = JS_NewContext(1L * 1024 * 1024);
  if (this->context) {
    //TODO: Throw java exception
  }

  if (!JS::InitSelfHostedCode(this->context)) {
    //TODO: Throw java exception
  }


//    printf("Debug JSGC_BYTES: %d\n", JS_GetGCParameter(this->context, JSGC_BYTES));
  printf("Debug JSGC_MAX_BYTES: %d\n", JS_GetGCParameter(this->context, JSGC_MAX_BYTES));
  JS_SetGCParameter(this->context, JSGC_MAX_BYTES, 32 * 1024 * 1024);
//  JS_SetGCParameter(this->context, JSGC_MAX_MALLOC_BYTES, 32*1024*1024);
  JS_SetNativeStackQuota(this->context, 256 * 1024);
  JS_SetGCParametersBasedOnAvailableMemory(this->context, 32 * 1024 * 1024);

  printf("Debug JSGC_MAX_BYTES: %d\n", JS_GetGCParameter(this->context, JSGC_MAX_BYTES));


  {
    JS::RealmOptions options;
    this->globalObject = new JS::RootedObject(
      this->context,
      JS_NewGlobalObject(
        this->context,
        &global_class,
        nullptr,
        JS::FireOnNewGlobalHook,
        options
      )
    );

    JSAutoRealm ar(this->context, *this->globalObject);
    if (!JS::InitRealmStandardClasses(this->context)) {
      // TODO: throw java exceptin
    }

    if (!JS_DefineFunction(this->context, *this->globalObject, "__yield", __yield, 1, 0)) {
      // TODO: throw java exception
    }
    if (!JS_DefineFunction(this->context, *this->globalObject, "__compile", __compile, 2, 0)) {
      // TODO: throw java exception
    }

    if (!JS_AddInterruptCallback(this->context, interruptCallback)) {
      // TODO: throw java exception
    }

    JS_SetContextPrivate(this->context, this);

    // First yield to get code to execute
    std::unordered_map<std::u16string, OCJS::JSValuePtr> map;
    map[u"type"] = OCJS::JSValuePtr(new OCJS::JSStringValue("__bios__"));
    OCJS::JSValuePtr src = this->yield(OCJS::JSValuePtr(new OCJS::JSMapValue(map)));

    OCJS::JSValuePtr res;
    if (src->getType() != OCJS::JSValue::Type::STRING) {
      // todo: throw java exception
      res = OCJS::JSValuePtr(new OCJS::JSStringValue("must provide a string for the first yield"));
    } else {
      res = this->compileAndExecute(src->asString()->value, u"__bios__");
    }
    this->deadResult = make_optional(res);
    if (this->outputPromise.has_value()) {
      this->outputPromise->set_value(res);
    }
    // We're now dead and can't execute anymore. :'(
    this->isDead = true;

    delete this->globalObject;
    this->globalObject = nullptr;
  }
  JS_DestroyContext(this->context);
  this->context = nullptr;

  debug_print(u"JS main thread end");

  this->engineLock.unlock();
}

OCJS::JSValuePtr SpiderMonkeyEngineNative::yield(const OCJS::JSValuePtr &output) {
  this->outputPromise->set_value(output);
  this->nextInput = nullopt;
  this->outputPromise = nullopt;
  this->engineWait.wait(this->engineLock, [this] {
    return this->nextInput.has_value();
  });
  fflush(stdout);
  return *this->nextInput;
}

OCJS::JSValuePtr SpiderMonkeyEngineNative::compileAndExecute(const u16string &src, const u16string &filename) {
  JSAutoRealm ar(this->context, *this->globalObject);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  string fileString = convert.to_bytes(filename);

  JS::CompileOptions compileOpts(this->context);
  compileOpts.setFileAndLine(fileString.c_str(), 1);
  JS::SourceText<char16_t> srcBuff;
  if (!srcBuff.init(this->context, src.c_str(), src.length(), JS::SourceOwnership::Borrowed)) {
    // TODO: throw error
    return OCJS::JSValuePtr(new OCJS::JSStringValue(u"src buff failed to init"));
  }

  JS::RootedValue res(this->context);
  if (!JS::Evaluate(this->context, compileOpts, srcBuff, &res)) {
    // TODO: throw error
    return OCJS::JSValuePtr(new OCJS::JSStringValue(u"evaluate failed"));
  }

  return convertObjectToJSValue(res);
}

bool SpiderMonkeyEngineNative::__yield(JSContext *ctx, unsigned argc, JS::Value *vp) {
  // Grab the engine
  auto *native = static_cast<SpiderMonkeyEngineNative *>(JS_GetContextPrivate(ctx));
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedValue val(ctx, args[0]);

  // pull out args
  OCJS::JSValuePtr json = native->convertObjectToJSValue(val);

  // yield
  OCJS::JSValuePtr res = native->yield(json);

  // Return
  if (!native->getJSValue(res, args.rval())) {
    JS_ReportErrorASCII(ctx, "Failed to convert JS");
    return false;
  }
  return true;
}

bool SpiderMonkeyEngineNative::__compile(JSContext *ctx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // pull out args
  string filenameVal = getU8String(ctx, args[0]);
  u16string srcVal = getU16String(ctx, args[1]);

  JS::CompileOptions compileOpts(ctx);
  compileOpts.setFileAndLine(filenameVal.c_str(), 1);
  compileOpts.setMutedErrors(false);
  JS::SourceText<char16_t> srcBuff;
  int res;
  if (!(res = srcBuff.init(ctx, srcVal.c_str(), srcVal.length(), JS::SourceOwnership::Borrowed))) {
    JS_ReportErrorASCII(ctx, "Failed to load source with error code %d", res);
    return false;
  }
  JS::RootedObjectVector emptyScopeChain(ctx);
  JS::RootedFunction func(ctx, JS::CompileFunction(
    ctx,
    emptyScopeChain,
    compileOpts,
    "__compile",
    0,
    nullptr,
    srcBuff
  ));

  if (!func) {
    return false;
  }
  args.rval().setObjectOrNull(JS_GetFunctionObject(func));
  return true;
}

bool SpiderMonkeyEngineNative::interruptCallback(JSContext *ctx) {
  auto *native = static_cast<SpiderMonkeyEngineNative *>(JS_GetContextPrivate(ctx));

  printf("Interrupt callback: %d\n", !native->shouldKill);
  //TODO: check timer and stuff
  return !native->shouldKill;
}

string SpiderMonkeyEngineNative::getU8String(JSContext *ctx, const JS::HandleValue &handle) {
  // can error with JS_ReportError
  JS::RootedString str(ctx, JS::ToString(ctx, handle));
  if (!str) return "failed to get u8 str";// TODO: throw error?
  JSFlatString *flatString = JS_FlattenString(ctx, str);
  if (!flatString) return "failed to flatten u8 str";// TODO: throw error?
  size_t len = JS::GetDeflatedUTF8StringLength(flatString);
  if (len > MAX_STR_SIZE) return "u8str too long";// TODO: throw error

//  JS::UniqueChars strChars = JS_EncodeStringToUTF8(ctx, str);
  string res;
  res.resize(len);
  JS::DeflateStringToUTF8Buffer(flatString, mozilla::RangedPtr<char>(res.data(), res.length()));
  return res;
}

u16string SpiderMonkeyEngineNative::getU16String(JSContext *ctx, const JS::HandleValue &handle) {
  JS::RootedString str(ctx, JS::ToString(ctx, handle));
  if (!str) return u"failed to get u16 str";// TODO: throw error?
  size_t len = JS_GetStringLength(str);
  if (len > MAX_STR_SIZE) return u"u16str too long";// TODO: throw error
  u16string res;
  res.resize(len);
  if (!JS_CopyStringChars(ctx, mozilla::Range<char16_t>(res.data(), res.length()), str)) {
    return u"failed to copy u16str"; // todo: throw error
  }
  return res;
}

void SpiderMonkeyEngineNative::debug_print(const std::u16string &str) {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  string fileString = convert.to_bytes(str);
  printf("%s\n", fileString.c_str());
  fflush(stdout);
}

size_t SpiderMonkeyEngineNative::getMaxMemory() const {
  return 0;
}

void SpiderMonkeyEngineNative::setMaxMemory(size_t maxMemory) {

}

size_t SpiderMonkeyEngineNative::getAllocatedMemory() const {
  return 0;
}

bool SpiderMonkeyEngineNative::getJSValue(const OCJS::JSValuePtr &ptr, JS::MutableHandleValue vp) {
  JSContext *ctx = this->context;

  switch (ptr->getType()) {
    case OCJS::JSValue::Type::STRING: {
      auto &str = ptr->asString()->value;
      vp.setString(JS_NewUCStringCopyN(ctx, str.c_str(), str.length()));
      return true;
    }
    case OCJS::JSValue::Type::BOOLEAN: {
      vp.setBoolean(ptr->asBoolean()->value);
      return true;
    }
    case OCJS::JSValue::Type::DOUBLE: {
      vp.setDouble(ptr->asDouble()->value);
      return true;
    }
    case OCJS::JSValue::Type::ARRAY: {
      auto &arr = ptr->asArray()->value;

      JS::RootedObject arrayObj(ctx, JS_NewArrayObject(ctx, arr.size()));

      for (size_t i = 0; i < arr.size(); i++) {
        JS::RootedValue ele(ctx);
        //TODO: am I ok to return false here?
        if (!getJSValue(arr[i], &ele) || !JS_SetElement(ctx, arrayObj, i, ele)) {
          return false;
        }
      }

      vp.setObjectOrNull(arrayObj.get());
      return true;
    }
    case OCJS::JSValue::Type::BYTE_ARRAY: {
      auto &buf = ptr->asByteArray()->value;
      JS::RootedObject arrayBuff(ctx, JS::NewArrayBuffer(ctx, buf.size()));
      {
        bool shared = false;
        JS::AutoCheckCannotGC noGC;
        void *data = JS::GetArrayBufferData(arrayBuff, &shared, noGC);
        memcpy(data, buf.data(), buf.size());
      }

      vp.setObjectOrNull(JS_NewUint8ArrayWithBuffer(ctx, arrayBuff, 0, -1));
      return true;
    }
    case OCJS::JSValue::Type::MAP: {
      auto &map = ptr->asMap()->value;
      JS::RootedObject obj(ctx, JS_NewObject(ctx, nullptr));

      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

      for (auto &ele : map) {
        JS::RootedValue val(ctx);
        //TODO: am I ok to return false here?
        if (!getJSValue(ele.second, &val) || !JS_SetUCProperty(ctx, obj, ele.first.c_str(), ele.first.length(), val)) {
          return false;
        }
      }

      vp.setObjectOrNull(obj);
      return true;
    }
    case OCJS::JSValue::Type::NULL_TYPE: {
      vp.setNull();
      return true;
    }
    default: {
      debug_print(u"Getting nothing?!?");
      vp.setNull();
      return false;
    }
  }
}

OCJS::JSValuePtr SpiderMonkeyEngineNative::convertObjectToJSValue(const JS::HandleValue &val) {
  JSContext *ctx = this->context;

  JSType type = JS_TypeOfValue(ctx, val);


  switch (type) {
    case JSTYPE_OBJECT: {
      JS::RootedObject obj(ctx, val.toObjectOrNull());

      // order matters, since an array buffer *is* an array, at least in some situations
      if (JS_IsArrayBufferViewObject(obj)) {
        size_t size = JS_GetArrayBufferViewByteLength(obj);
        JS::AutoCheckCannotGC noGC;
        bool isShared = false;
        void *data = JS_GetArrayBufferViewData(obj, &isShared, noGC);

        std::vector<uint8_t> resBuff;
        resBuff.resize(size);
        memcpy(resBuff.data(), data, size);
        return OCJS::JSValuePtr(new OCJS::JSByteArrayValue(resBuff));
      }

      if (JS_IsTypedArrayObject(obj)) {
        size_t size = JS_GetTypedArrayByteLength(obj);
        JS::AutoCheckCannotGC noGC;
        bool isShared = false;
        uint8_t *data = JS_GetUint8ArrayData(obj, &isShared, noGC);

        std::vector<uint8_t> resBuff;
        resBuff.resize(size);
        memcpy(resBuff.data(), data, size);
        return OCJS::JSValuePtr(new OCJS::JSByteArrayValue(resBuff));
      }

      if (JS::IsArrayBufferObject(obj)) {
        uint32_t len = 0;
        bool isShared = false;
        unsigned char *data = nullptr;
        JS::GetArrayBufferLengthAndData(obj, &len, &isShared, &data);

        std::vector<uint8_t> resBuff;
        resBuff.resize(len);
        memcpy(resBuff.data(), data, len);
        return OCJS::JSValuePtr(new OCJS::JSByteArrayValue(resBuff));
      }

      JS::IsArrayAnswer isArray = JS::IsArrayAnswer::NotArray;
      if (!JS::IsArray(ctx, obj, &isArray)) {
        return OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to check if is array (this shouldn't happen!)"));
      }
      if (isArray == JS::IsArrayAnswer::Array) {
        uint32_t len = 0;
        if (!JS_GetArrayLength(ctx, obj, &len)) {
          return OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to get array length (this shouldn't happen!)"));
        }
        std::vector<OCJS::JSValuePtr> resVec;
        resVec.resize(len);
        for (size_t i = 0; i < len; i++) {
          JS::RootedValue ele(ctx);
          if (!JS_GetElement(ctx, obj, i, &ele)) {
            resVec[i] = OCJS::JSValuePtr(
              new OCJS::JSStringValue("Failed to get array element (this shouldn't happen!)"));
          } else {
            resVec[i] = convertObjectToJSValue(ele);
          }
        }
        return OCJS::JSValuePtr(new OCJS::JSArrayValue(resVec));
      }

      {
        // just a regular ol' object
        std::unordered_map<std::u16string, OCJS::JSValuePtr> res;

        JS::Rooted<JS::IdVector> ids(ctx, JS::IdVector(ctx));
        if (!JS_Enumerate(ctx, obj, &ids)) {
          return OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to enumerate object (this shouldn't happen!)"));
        }

        for (size_t i = 0; i < ids.length(); i++) {
          JS::RootedId id(ctx, ids[i]);
          JS::RootedValue eleID(ctx);
          if (!JS_IdToValue(ctx, id, &eleID)) {
            return OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to get key value (this shouldn't happen!)"));
          }
          JS::RootedString eleIDStr(ctx, JS::ToString(ctx, eleID));
          std::u16string eleKey;
          eleKey.resize(JS_GetStringLength(eleIDStr));
          if (!JS_CopyStringChars(ctx, mozilla::Range<char16_t>(eleKey.data(), eleKey.length()), eleIDStr)) {
            return OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to get string chars (this shouldn't happen!)"));
          }

          JS::RootedValue ele(ctx);
          if (!JS_GetPropertyById(ctx, obj, id, &ele)) {
            res[eleKey] = OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to get ele value (this shouldn't happen!)"));
          } else {
            res[eleKey] = convertObjectToJSValue(ele);
          }
        }

        return OCJS::JSValuePtr(new OCJS::JSMapValue(res));
      }
    }
    case JSTYPE_SYMBOL:
    case JSTYPE_STRING: {
      JS::RootedString str(ctx, JS::ToString(ctx, val));
      std::u16string res;
      res.resize(JS_GetStringLength(str));
      if (!JS_CopyStringChars(ctx, mozilla::Range<char16_t>(res.data(), res.length()), str)) {
        return OCJS::JSValuePtr(new OCJS::JSStringValue("Failed to get string chars (this shouldn't happen!)"));
      }

      return OCJS::JSValuePtr(new OCJS::JSStringValue(res));
    }
    case JSTYPE_BIGINT:
    case JSTYPE_NUMBER: {
      double d = NAN;
      if (!JS::ToNumber(ctx, val, &d)) {
        d = NAN;
      }
      return OCJS::JSValuePtr(new OCJS::JSDoubleValue(d));
    }
    case JSTYPE_BOOLEAN: {
      return OCJS::JSValuePtr(new OCJS::JSBooleanValue(JS::ToBoolean(val)));
    }
    case JSTYPE_FUNCTION:
    case JSTYPE_UNDEFINED:
    case JSTYPE_NULL:
    case JSTYPE_LIMIT:
    default: {
      return OCJS::JSValuePtr(new OCJS::JSNullValue());
    }
  }
}
