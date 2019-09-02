//
// Created by pwootage on 11/14/17.
//
#include "SpiderMonkeyEngineNative.h"
#include <js/Initialization.h>
#include <js/Conversions.h>
#include <js/JSON.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>

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

jfieldID spiderMonkeyEngineNativeFID = nullptr;

void SpiderMonkeyEngineNative::Initialize(JNIEnv *env, jclass clazz) {
  JS_Init();

  jclass v8EngineClass = env->FindClass("com/pwootage/oc/js/spidermonkey/SpiderMonkeyEngine");
  spiderMonkeyEngineNativeFID = env->GetFieldID(v8EngineClass, "spiderMonkeyEngineNative", "J");
}

SpiderMonkeyEngineNative *SpiderMonkeyEngineNative::getFromJava(JNIEnv *env, jobject obj) {
  return reinterpret_cast<SpiderMonkeyEngineNative *>(env->GetLongField(obj, spiderMonkeyEngineNativeFID));
}

void SpiderMonkeyEngineNative::setToJava(JNIEnv *env, jobject obj, SpiderMonkeyEngineNative *data) {
  env->SetLongField(obj, spiderMonkeyEngineNativeFID, reinterpret_cast<jlong>(data));
}

SpiderMonkeyEngineNative::SpiderMonkeyEngineNative(JNIEnv *env, jobject obj) {
  env->GetJavaVM(&javaVM);
  globalObjRef = env->NewGlobalRef(obj);

  this->mainThread = thread([this] { this->mainThreadFn(); });
}

SpiderMonkeyEngineNative::~SpiderMonkeyEngineNative() {
  this->next(uR"({"state": "error", "value": "kill"})");
  shouldKill = true;
  JS_RequestInterruptCallback(this->context);
  this->mainThread.join();
  //TODO: safe/not busy wait
  while (!isDead) {
    // wait
  }

  delete this->globalObject;
  this->globalObject = nullptr;
  JS_DestroyContext(this->context);
  this->context = nullptr;
}

SpiderMonkeyEngineNative::JNIPtr SpiderMonkeyEngineNative::getEnv() {
  bool detach = false;
  JNIEnv *env = nullptr;
  // double check it's all ok
  int getEnvStat = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
  if (getEnvStat == JNI_EDETACHED) {
    detach = true;
    if (javaVM->AttachCurrentThread((void **) &env, NULL) != 0) {
      cout << "Failed to attach" << endl;
    }
  } else if (getEnvStat == JNI_OK) {
    detach = false;
  }

  return JNIPtr(env, [this](JNIEnv *ptr) {
//      if (detach) {
    this->javaVM->DetachCurrentThread();
//      }
  });
}

future<u16string> SpiderMonkeyEngineNative::next(u16string next) {
  future<u16string> res;
  {
    lock_guard<mutex> lock(this->executionMutex);

    if (this->deadResult) {
      promise<u16string> promise;
      promise.set_value(*this->deadResult);
      return promise.get_future();
    }

    this->nextInput = make_optional(next);
    this->outputPromise = make_optional(promise<u16string>());
    res = this->outputPromise->get_future();
  }
  this->engineWait.notify_one();
  return res;
}

void SpiderMonkeyEngineNative::mainThreadFn() {
  //init
  this->context = JS_NewContext(1L * 1024 * 1024);
  if (this->context) {
    //TODO: Throw java exception
  }
  
  if (!JS::InitSelfHostedCode(this->context)) {
    //TODO: Throw java exception
  }

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
  };

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

  // Lock the engine
  this->engineLock = unique_lock(this->executionMutex);

  // First yield to get code to execute
  u16string src = this->yield(u"{\"state\": \"__bios__\"}");
  u16string res = this->compileAndExecute(src, u"__bios__");
  this->deadResult = make_optional(res);
  if (this->outputPromise.has_value()) {
    this->outputPromise->set_value(res);
  }
  // We're now dead and can't execute anymore. :'(
  this->engineLock.unlock();
  this->engineLock = unique_lock<mutex>();
}

u16string SpiderMonkeyEngineNative::yield(const u16string &output) {
  this->outputPromise->set_value(output);
  this->nextInput = nullopt;
  this->outputPromise = nullopt;
  this->engineWait.wait(this->engineLock, [this] {
    return this->nextInput.has_value();
  });
  fflush(stdout);
  return *this->nextInput;
}

u16string SpiderMonkeyEngineNative::compileAndExecute(u16string src, u16string filename) {
  auto env = getEnv();

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
  string fileString = convert.to_bytes(filename);

  JS::CompileOptions compileOpts(this->context);
  compileOpts.setFileAndLine(fileString.c_str(), 1);
  JS::SourceText<char16_t> srcBuff;
  if (!srcBuff.init(this->context, src.c_str(), src.length(), JS::SourceOwnership::Borrowed)) {
    // TODO: throw error
    return u"";
  }

  JS::RootedValue res(this->context);
  if (!JS::Evaluate(this->context, compileOpts, srcBuff, &res)) {
    // TODO: throw error
    return u"";
  }

  //TODO: catch js exception


  JS::RootedString resStr(this->context, JS::ToString(this->context, res));

  if (!resStr) {
    // TODO: throw error
    return u"";
  }

  // Usual return type: {"type": "exec_cmd", "result": <whatever it returned>}

  size_t len = JS_GetStringLength(resStr);
  if (len > MAX_STR_SIZE) return u"";// TODO: throw error
  return u16string(JS_GetTwoByteExternalStringChars(resStr), len);
}

//Local <Object> SpiderMonkeyEngineNative::convertException(Local <Context> context, TryCatch &tryCatch) {
//  EscapableHandleScope handle_scope(isolate);
//  Local <Message> message = tryCatch.Message();
//  if (message.IsEmpty()) {
//    return handle_scope.Escape(Object::New(isolate));
//  }
//
//  Local <Object> result = Object::New(isolate);
//  result->Set(
//    context,
//    String::NewFromUtf8(isolate, "type", NewStringType::kNormal).ToLocalChecked(),
//    String::NewFromUtf8(isolate, "error", NewStringType::kNormal).ToLocalChecked()
//  ).ToChecked();
//  result->Set(
//    context,
//    String::NewFromUtf8(isolate, "file", NewStringType::kNormal).ToLocalChecked(),
//    message->GetScriptResourceName()
//  ).ToChecked();
//  MaybeLocal <String> sourceLine = message->GetSourceLine(context);
//  if (!sourceLine.IsEmpty() && !sourceLine.IsEmpty() && !sourceLine.ToLocalChecked().IsEmpty()) {
//    result->Set(
//      context,
//      String::NewFromUtf8(isolate, "src", NewStringType::kNormal).ToLocalChecked(),
//      sourceLine.ToLocalChecked()
//    ).ToChecked();
//  }
//  Maybe<int> line = message->GetLineNumber(context);
//  if (line.IsJust()) {
//    result->Set(
//      context,
//      String::NewFromUtf8(isolate, "line", NewStringType::kNormal).ToLocalChecked(),
//      Integer::New(isolate, line.FromJust())
//    ).ToChecked();
//  }
//  Maybe<int> startCol = message->GetStartColumn(context);
//  if (startCol.IsJust()) {
//    result->Set(
//      context,
//      String::NewFromUtf8(isolate, "start", NewStringType::kNormal).ToLocalChecked(),
//      Integer::New(isolate, startCol.FromJust())
//    ).ToChecked();
//  }
//  Maybe<int> endCol = message->GetStartColumn(context);
//  if (endCol.IsJust()) {
//    result->Set(
//      context,
//      String::NewFromUtf8(isolate, "end", NewStringType::kNormal).ToLocalChecked(),
//      Integer::New(isolate, endCol.FromJust())
//    ).ToChecked();
//  }
//
//  Local <StackTrace> stackTrace = message->GetStackTrace();
//  if (!stackTrace.IsEmpty()) {
//    Local <Array> stackArray = Array::New(isolate, stackTrace->GetFrameCount());
//    for (uint32_t i = 0; i < static_cast<uint32_t>(stackTrace->GetFrameCount()); i++) {
//      Local <StackFrame> frame = stackTrace->GetFrame(i);
//      Local <Object> resFrame = Object::New(isolate);
//      resFrame->Set(
//        context,
//        String::NewFromUtf8(isolate, "file", NewStringType::kNormal).ToLocalChecked(),
//        frame->GetScriptName()
//      ).ToChecked();
//      resFrame->Set(
//        context,
//        String::NewFromUtf8(isolate, "function", NewStringType::kNormal).ToLocalChecked(),
//        frame->GetFunctionName()
//      ).ToChecked();
//      resFrame->Set(
//        context,
//        String::NewFromUtf8(isolate, "line", NewStringType::kNormal).ToLocalChecked(),
//        Integer::New(isolate, frame->GetLineNumber())
//      ).ToChecked();
//      resFrame->Set(
//        context,
//        String::NewFromUtf8(isolate, "col", NewStringType::kNormal).ToLocalChecked(),
//        Integer::New(isolate, frame->GetColumn())
//      ).ToChecked();
//      stackArray->Set(context, i, resFrame).ToChecked();
//    }
//
//    result->Set(
//      context,
//      String::NewFromUtf8(isolate, "stacktrace", NewStringType::kNormal).ToLocalChecked(),
//      stackArray
//    ).ToChecked();
//  }
//
//  result->Set(
//    context,
//    String::NewFromUtf8(isolate, "origin", NewStringType::kNormal).ToLocalChecked(),
//    message->GetScriptOrigin().ResourceName()
//  ).ToChecked();
//  result->Set(
//    context,
//    String::NewFromUtf8(isolate, "message", NewStringType::kNormal).ToLocalChecked(),
//    message->Get()
//  ).ToChecked();
//  return handle_scope.Escape(result);
//}

bool SpiderMonkeyEngineNative::__yield(JSContext *ctx, unsigned argc, JS::Value *vp) {
  // Grab the engine
  auto *native = static_cast<SpiderMonkeyEngineNative *>(JS_GetContextPrivate(ctx));
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // pull out args
  u16string json = getU16String(ctx, args[0]);

  // yield
  u16string res = native->yield(json);

  // Return
  args.rval().setString(JS_NewUCStringCopyN(ctx, res.c_str(), res.length()));
  return true;
}

bool SpiderMonkeyEngineNative::__compile(JSContext *ctx, unsigned argc, JS::Value *vp) {
//  auto *native = static_cast<SpiderMonkeyEngineNative *>(JS_GetContextPrivate(ctx));
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // pull out args
  string filenameVal = getU8String(ctx, args[0]);
  u16string srcVal = getU16String(ctx, args[1]);

  JS::CompileOptions compileOpts(ctx);
  compileOpts.setFileAndLine(filenameVal.c_str(), 1);
  JS::SourceText<char16_t> srcBuff;
  if (!srcBuff.init(ctx, srcVal.c_str(), srcVal.length(), JS::SourceOwnership::Borrowed)) {
    // TODO: throw error
    return false;
  }
  // TODO: error handling?
  JS::RootedObjectVector emptyScopeChain(ctx);
  JS::RootedFunction func(ctx, JS::CompileFunction(ctx, emptyScopeChain, compileOpts, "__compiled_func", 0, nullptr, srcBuff));
  args.rval().setObject(*JS_GetFunctionObject(func));
  return true;
}

bool SpiderMonkeyEngineNative::interruptCallback(JSContext *ctx) {
  auto *native = static_cast<SpiderMonkeyEngineNative *>(JS_GetContextPrivate(ctx));

  //TODO: check timer and stuff
  return native->shouldKill;
}

string SpiderMonkeyEngineNative::getU8String(JSContext *ctx, const JS::HandleValue &handle) {
  JS::RootedString str(ctx, JS::ToString(ctx, handle));
  if (!str) return "";// TODO: throw error?
  JSFlatString *flatString = JS_FlattenString(ctx, str);
  if (!flatString) return "";// TODO: throw error?
  size_t len = JS::GetDeflatedUTF8StringLength(flatString);
  if (len > MAX_STR_SIZE) return "";// TODO: throw error

//  JS::UniqueChars strChars = JS_EncodeStringToUTF8(ctx, str);
  string res;
  res.resize(len);
  JS::DeflateStringToUTF8Buffer(flatString, mozilla::RangedPtr<char>(res.data(), res.length()));
  return res;
}

u16string SpiderMonkeyEngineNative::getU16String(JSContext *ctx, const JS::HandleValue &handle) {
  JS::RootedString str(ctx, JS::ToString(ctx, handle));
  if (!str) return u"";// TODO: throw error?
  size_t len = JS_GetStringLength(str);
  if (len > MAX_STR_SIZE) return u"";// TODO: throw error
  u16string res;
  res.resize(len);
  if (!JS_CopyStringChars(ctx, mozilla::Range<char16_t>(res.data(), res.length()), str)) {
    return u""; // todo: throw error
  }
  return res;
}
