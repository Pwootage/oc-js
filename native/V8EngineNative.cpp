//
// Created by pwootage on 11/14/17.
//
#include "V8EngineNative.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <functional>

using namespace v8;
using namespace std;

Platform *v8Platform = nullptr;
jfieldID v8EngineNativeFID = nullptr;
StartupData *icudtl_data;
StartupData *natives_blob_data;
StartupData *snapshot_blob_data;

StartupData *getData(JNIEnv *env, jobject buff) {
  StartupData *res = new StartupData;

  jlong size = env->GetDirectBufferCapacity(buff);
  const void *rawData = env->GetDirectBufferAddress(buff);
  char *data = new char[size];
  memcpy(data, rawData, static_cast<size_t>(size));

  res->raw_size = static_cast<int>(size);
  res->data = data;

  return res;
}


void V8EngineNative::Initialize(JNIEnv *env, jclass clazz) {
  jfieldID icudtl_id = env->GetStaticFieldID(clazz, "icudtl", "Ljava/nio/ByteBuffer;");
  jfieldID snapshot_blob_id = env->GetStaticFieldID(clazz, "snapshot_blob", "Ljava/nio/ByteBuffer;");
  jfieldID natives_blob_id = env->GetStaticFieldID(clazz, "natives_blob", "Ljava/nio/ByteBuffer;");
  jobject icudtl = env->GetStaticObjectField(clazz, icudtl_id);
  jobject natives_blob = env->GetStaticObjectField(clazz, natives_blob_id);
  jobject snapshot_blob = env->GetStaticObjectField(clazz, snapshot_blob_id);


  icudtl_data = getData(env, icudtl);
  natives_blob_data = getData(env, natives_blob);
  snapshot_blob_data = getData(env, snapshot_blob);

  // TODO: icudtl?
  V8::SetNativesDataBlob(natives_blob_data);
  V8::SetSnapshotDataBlob(snapshot_blob_data);
  v8Platform = platform::CreateDefaultPlatform(1,
                                               platform::IdleTaskSupport::kDisabled,
                                               platform::InProcessStackDumping::kDisabled);
  V8::InitializePlatform(v8Platform);
  V8::Initialize();

  jclass v8EngineClass = env->FindClass("com/pwootage/oc/js/v8/V8Engine");
  v8EngineNativeFID = env->GetFieldID(v8EngineClass, "v8EngineNative", "J");
}

V8EngineNative *V8EngineNative::getFromJava(JNIEnv *env, jobject obj) {
  return reinterpret_cast<V8EngineNative *>(env->GetLongField(obj, v8EngineNativeFID));
}

void V8EngineNative::setToJava(JNIEnv *env, jobject obj, V8EngineNative *data) {
  env->SetLongField(obj, v8EngineNativeFID, reinterpret_cast<jlong>(data));
}

V8EngineNative::V8EngineNative(JNIEnv *env, jobject obj) {
  env->GetJavaVM(&javaVM);
  globalObjRef = env->NewGlobalRef(obj);

  create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate = Isolate::New(create_params);
  Locker locker(isolate);
  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    //TODO: attach global stuff to this template
    Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
    contextRef.Reset(isolate, Context::New(isolate, nullptr, global));
    Local<Context> context = contextRef.Get(isolate);

    Context::Scope context_scope(context);

    {
      Local<ObjectTemplate> __yieldTemplate = ObjectTemplate::New(isolate);
      __yieldTemplate->SetInternalFieldCount(1);
      __yieldTemplate->SetCallAsFunctionHandler(__yield);
      __yieldTemplate->SetImmutableProto();
      Local<Object> v8Native = __yieldTemplate->NewInstance(context).ToLocalChecked();
      v8Native->SetInternalField(0, External::New(isolate, this));
      context->Global()->Set(
          context,
          String::NewFromUtf8(isolate, "__yield", NewStringType::kNormal).ToLocalChecked(),
          v8Native
      ).ToChecked();
    }
    {
      Local<ObjectTemplate> __compileTemplate = ObjectTemplate::New(isolate);
      __compileTemplate->SetInternalFieldCount(1);
      __compileTemplate->SetCallAsFunctionHandler(__compile);
      __compileTemplate->SetImmutableProto();
      Local<Object> v8Native = __compileTemplate->NewInstance(context).ToLocalChecked();
      v8Native->SetInternalField(0, External::New(isolate, this));
      context->Global()->Set(
          context,
          String::NewFromUtf8(isolate, "__compile", NewStringType::kNormal).ToLocalChecked(),
          v8Native
      ).ToChecked();
    }
  }

  this->mainThread = thread([this]{this->mainThreadFn();});
}

V8EngineNative::~V8EngineNative() {
  this->next("{\"state\": \"error\", \"value\": \"kill\"}");
  isolate->TerminateExecution();
  while (isolate->IsExecutionTerminating()) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }
  this->mainThread.join();
  contextRef.Reset();
  isolate->Dispose();
  delete create_params.array_buffer_allocator;

}

V8EngineNative::JNIPtr V8EngineNative::getEnv() {
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

future<string> V8EngineNative::next(string next) {
  future<string> res;
  {
    lock_guard<mutex> lock(this->executionMutex);

    if (this->deadResult) {
      promise<string> promise;
      promise.set_value(*this->deadResult);
      return promise.get_future();
    }

    this->nextInput = make_optional(next);
    this->outputPromise = make_optional(promise<string>());
    res = this->outputPromise->get_future();
  }
  this->engineWait.notify_one();
  return res;
}

void V8EngineNative::mainThreadFn() {
  Locker locker(isolate);
  Isolate::Scope isolateScope(isolate);
  HandleScope handleScope(isolate);
  Local<Context> context = this->contextRef.Get(isolate);
  Context::Scope contextScope(context);

  // Lock the engine
  this->engineLock = unique_lock(this->executionMutex);

  // First yield to get code to execute
  string src = this->yield("{\"state\": \"__bios__\"}");
  string res = this->compileAndExecute(src, "__bios__");
  this->deadResult = make_optional(res);
  if (this->outputPromise.has_value()) {
    this->outputPromise->set_value(res);
  }
  // We're now dead and can't execute anymore. :'(
  this->engineLock.unlock();
  this->engineLock = unique_lock<mutex>();
}

string V8EngineNative::yield(string output) {
  this->outputPromise->set_value(output);
  this->nextInput = nullopt;
  this->outputPromise = nullopt;
  this->engineWait.wait(this->engineLock, [this]{
      return this->nextInput.has_value();
  });
  fflush(stdout);
  return *this->nextInput;
}

string V8EngineNative::compileAndExecute(string src, string filename) {
  auto env = getEnv();

  Isolate::Scope isolate_scope(isolate);
  EscapableHandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);
  Local<String> srcStr = String::NewFromUtf8(isolate, src.c_str(), NewStringType::kNormal).ToLocalChecked();
  Local<String> filenameStr = String::NewFromUtf8(isolate, filename.c_str(), NewStringType::kNormal).ToLocalChecked();

  Local<Context> context = contextRef.Get(isolate);
  Context::Scope contextScope(context);

  ScriptOrigin origin(filenameStr);
  MaybeLocal<Script> script = Script::Compile(context, srcStr, &origin);
  if (script.IsEmpty() || script.ToLocalChecked().IsEmpty()) {
    Local<Object> res = convertException(context, try_catch);
    MaybeLocal<String> jsonStr = JSON::Stringify(context, res);
    if (jsonStr.IsEmpty() || jsonStr.ToLocalChecked().IsEmpty()) {
      return "{\"type\": \"error\", \"message\": \"Probably a threading problem...\"}";
    } else {
      return string(*String::Utf8Value(isolate, jsonStr.ToLocalChecked()));
    }
  }

  MaybeLocal<Value> scriptResult = script.ToLocalChecked()->Run(context);
  if (scriptResult.IsEmpty() || scriptResult.ToLocalChecked().IsEmpty()) {
    Local<Value> res = convertException(context, try_catch);
    Local<String> jsonStr = JSON::Stringify(context, res).ToLocalChecked();
    return string(*String::Utf8Value(isolate, jsonStr));
  }

  Local<Object> res = Object::New(isolate);
  res->Set(
      context,
      String::NewFromUtf8(isolate, "type", NewStringType::kNormal).ToLocalChecked(),
      String::NewFromUtf8(isolate, "exec_end", NewStringType::kNormal).ToLocalChecked()
  ).ToChecked();
  res->Set(
      context,
      String::NewFromUtf8(isolate, "result", NewStringType::kNormal).ToLocalChecked(),
      scriptResult.ToLocalChecked()
  ).ToChecked();
  Local<String> jsonStr = JSON::Stringify(context, res).ToLocalChecked();
  return string(*String::Utf8Value(isolate, jsonStr));
}

Isolate *V8EngineNative::getIsolate() {
  return isolate;
}

Local<Object> V8EngineNative::convertException(Local<Context> context, TryCatch &tryCatch) {
  EscapableHandleScope handle_scope(isolate);
  Local<Message> message = tryCatch.Message();
  if (message.IsEmpty()) {
    return handle_scope.Escape(Object::New(isolate));
  }

  Local<Object> result = Object::New(isolate);
  result->Set(
      context,
      String::NewFromUtf8(isolate, "type", NewStringType::kNormal).ToLocalChecked(),
      String::NewFromUtf8(isolate, "error", NewStringType::kNormal).ToLocalChecked()
  ).ToChecked();
  result->Set(
      context,
      String::NewFromUtf8(isolate, "file", NewStringType::kNormal).ToLocalChecked(),
      message->GetScriptResourceName()
  ).ToChecked();
  MaybeLocal<String> sourceLine = message->GetSourceLine(context);
  if (!sourceLine.IsEmpty() && !sourceLine.IsEmpty() && !sourceLine.ToLocalChecked().IsEmpty()) {
    result->Set(
        context,
        String::NewFromUtf8(isolate, "src", NewStringType::kNormal).ToLocalChecked(),
        sourceLine.ToLocalChecked()
    ).ToChecked();
  }
  Maybe<int> line = message->GetLineNumber(context);
  if (line.IsJust()) {
    result->Set(
        context,
        String::NewFromUtf8(isolate, "line", NewStringType::kNormal).ToLocalChecked(),
        Integer::New(isolate, line.FromJust())
    ).ToChecked();
  }
  Maybe<int> startCol = message->GetStartColumn(context);
  if (startCol.IsJust()) {
    result->Set(
        context,
        String::NewFromUtf8(isolate, "start", NewStringType::kNormal).ToLocalChecked(),
        Integer::New(isolate, startCol.FromJust())
    ).ToChecked();
  }
  Maybe<int> endCol = message->GetStartColumn(context);
  if (endCol.IsJust()) {
    result->Set(
        context,
        String::NewFromUtf8(isolate, "end", NewStringType::kNormal).ToLocalChecked(),
        Integer::New(isolate, endCol.FromJust())
    ).ToChecked();
  }

  Local<StackTrace> stackTrace = message->GetStackTrace();
  if (!stackTrace.IsEmpty()) {
    Local<Array> stackArray = Array::New(isolate, stackTrace->GetFrameCount());
    for (uint32_t i = 0; i < static_cast<uint32_t>(stackTrace->GetFrameCount()); i++) {
      Local<StackFrame> frame = stackTrace->GetFrame(i);
      Local<Object> resFrame = Object::New(isolate);
      resFrame->Set(
          context,
          String::NewFromUtf8(isolate, "file", NewStringType::kNormal).ToLocalChecked(),
          frame->GetScriptName()
      ).ToChecked();
      resFrame->Set(
          context,
          String::NewFromUtf8(isolate, "function", NewStringType::kNormal).ToLocalChecked(),
          frame->GetFunctionName()
      ).ToChecked();
      resFrame->Set(
          context,
          String::NewFromUtf8(isolate, "line", NewStringType::kNormal).ToLocalChecked(),
          Integer::New(isolate, frame->GetLineNumber())
      ).ToChecked();
      resFrame->Set(
          context,
          String::NewFromUtf8(isolate, "col", NewStringType::kNormal).ToLocalChecked(),
          Integer::New(isolate, frame->GetColumn())
      ).ToChecked();
      stackArray->Set(context, i, resFrame).ToChecked();
    }

    result->Set(
        context,
        String::NewFromUtf8(isolate, "stacktrace", NewStringType::kNormal).ToLocalChecked(),
        stackArray
    ).ToChecked();
  }

  result->Set(
      context,
      String::NewFromUtf8(isolate, "origin", NewStringType::kNormal).ToLocalChecked(),
      message->GetScriptOrigin().ResourceName()
  ).ToChecked();
  result->Set(
      context,
      String::NewFromUtf8(isolate, "message", NewStringType::kNormal).ToLocalChecked(),
      message->Get()
  ).ToChecked();
  return handle_scope.Escape(result);
}

void V8EngineNative::__yield(const FunctionCallbackInfo<Value> &info) {
  // Grab the engine
  Local<Object> jsThis = info.Holder();
  Local<External> wrap = Local<External>::Cast(jsThis->GetInternalField(0));
  V8EngineNative *ptr = static_cast<V8EngineNative *>(wrap->Value());
  // Make sure we don't leak
  HandleScope handle_scope(info.GetIsolate());
  // Pull in yield arg
  Local<Value> arg = info[0];
  Local<String> jsonLocal = JSON::Stringify(info.Holder()->CreationContext(), arg).ToLocalChecked();
  string json(*String::Utf8Value(info.GetIsolate(), jsonLocal));

  // Actually yield
  string res = ptr->yield(json);

  // Return the yield result
  Local<String> resLocalStr = String::NewFromUtf8(info.GetIsolate(), res.c_str(), NewStringType::kNormal).ToLocalChecked();
  info.GetReturnValue().Set(resLocalStr);
}

void V8EngineNative::__compile(const FunctionCallbackInfo<Value> &info) {
  // Grab the engine
  Local<Object> jsThis = info.Holder();
  // Make sure we don't leak
  HandleScope handle_scope(info.GetIsolate());
  // Pull in yield arg
  Local<Value> filenameVal = info[0];
  Local<Value> srcVal = info[1];

  if (!filenameVal->IsString()) {
    info.GetIsolate()->ThrowException(
        String::NewFromUtf8(info.GetIsolate(), "Compile filename must be a string", NewStringType::kNormal).ToLocalChecked()
    );
    return;
  }
  if (!srcVal->IsString()) {
    info.GetIsolate()->ThrowException(
        String::NewFromUtf8(info.GetIsolate(), "Compile src must be a string", NewStringType::kNormal).ToLocalChecked()
    );
    return;
  }

  // These are checked
  Local<String> filename = Local<String>::Cast(filenameVal);
  Local<String> src = Local<String>::Cast(srcVal);

  TryCatch tryCatch(info.GetIsolate());
  ScriptOrigin origin(filename);
  MaybeLocal<Script> script = Script::Compile(jsThis->CreationContext(), src, &origin);
  if (script.IsEmpty() || tryCatch.HasCaught()) {
    tryCatch.ReThrow();
    return;
  }

  MaybeLocal<Value> res = script.ToLocalChecked()->Run(jsThis->CreationContext());
  if (res.IsEmpty() || tryCatch.HasCaught()) {
    tryCatch.ReThrow();
    return;
  }
  info.GetReturnValue().Set(res.ToLocalChecked());
}
