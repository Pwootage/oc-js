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
jmethodID v8EngineNative__call = nullptr;
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
  v8EngineNative__call = env->GetMethodID(v8EngineClass, "__call", "(Ljava/lang/String;)Ljava/lang/String;");
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
    Local<ObjectTemplate> v8NativeTemplate = ObjectTemplate::New(isolate);
    v8NativeTemplate->SetInternalFieldCount(1);
    v8NativeTemplate->SetCallAsFunctionHandler(__call);
    v8NativeTemplate->SetImmutableProto();


    contextRef.Reset(isolate, Context::New(isolate, nullptr, global));
    Local<Context> context = contextRef.Get(isolate);

    Context::Scope context_scope(context);
    Local<Object> v8Native = v8NativeTemplate->NewInstance(context).ToLocalChecked();
    v8Native->SetInternalField(0, External::New(isolate, this));
    context->Global()->Set(
        context,
        String::NewFromUtf8(isolate, "__bios_call", NewStringType::kNormal).ToLocalChecked(),
        v8Native
    ).ToChecked();
  }
}

V8EngineNative::~V8EngineNative() {
  isolate->TerminateExecution();
  while (isolate->IsExecutionTerminating()) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }
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

Local<String>
V8EngineNative::compileAndExecute(jstring src, jstring filename) {
  auto env = getEnv();

  Isolate::Scope isolate_scope(isolate);
  EscapableHandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);
  const char *srcCStr = env->GetStringUTFChars(src, JNI_FALSE);
  Local<String> srcStr = String::NewFromUtf8(isolate, srcCStr, NewStringType::kNormal).ToLocalChecked();
  env->ReleaseStringUTFChars(src, srcCStr);

  const char *filenameCStr = env->GetStringUTFChars(filename, JNI_FALSE);
  Local<String> filenameStr = String::NewFromUtf8(isolate, filenameCStr, NewStringType::kNormal).ToLocalChecked();
  env->ReleaseStringUTFChars(filename, filenameCStr);

  Local<Context> context = contextRef.Get(isolate);
    Context::Scope contextScope(context);

  ScriptOrigin origin(filenameStr);
  MaybeLocal<Script> script = Script::Compile(context, srcStr, &origin);
  if (script.IsEmpty() || script.ToLocalChecked().IsEmpty()) {
    Local<Object> res = convertException(context, try_catch);
    MaybeLocal<String> jsonStr = JSON::Stringify(context, res);
    if (jsonStr.IsEmpty() || jsonStr.ToLocalChecked().IsEmpty()) {
      return handle_scope.Escape(
          String::NewFromUtf8(isolate, "{\"state\": \"error\", \"message\": \"Probably a threading problem...\"}", NewStringType::kNormal).ToLocalChecked());
    } else {
      return handle_scope.Escape(jsonStr.ToLocalChecked());
    }
  }

  MaybeLocal<Value> scriptResult = script.ToLocalChecked()->Run(context);
  if (scriptResult.IsEmpty() || scriptResult.ToLocalChecked().IsEmpty()) {
    Local<Value> res = convertException(context, try_catch);
    Local<String> jsonStr = JSON::Stringify(context, res).ToLocalChecked();
    return handle_scope.Escape(jsonStr);
  }

  Local<Object> res = Object::New(isolate);
  res->Set(
      context,
      String::NewFromUtf8(isolate, "state", NewStringType::kNormal).ToLocalChecked(),
      String::NewFromUtf8(isolate, "success", NewStringType::kNormal).ToLocalChecked()
  ).ToChecked();
  res->Set(
      context,
      String::NewFromUtf8(isolate, "result", NewStringType::kNormal).ToLocalChecked(),
      scriptResult.ToLocalChecked()
  ).ToChecked();
  Local<String> jsonStr = JSON::Stringify(context, res).ToLocalChecked();
  return handle_scope.Escape(jsonStr);
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
      String::NewFromUtf8(isolate, "state", NewStringType::kNormal).ToLocalChecked(),
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

void V8EngineNative::__call(const FunctionCallbackInfo<Value> &info) {
  Local<Object> jsThis = info.Holder();
  Local<External> wrap = Local<External>::Cast(jsThis->GetInternalField(0));
  V8EngineNative *ptr = static_cast<V8EngineNative *>(wrap->Value());
  HandleScope handle_scope(info.GetIsolate());

  JNIPtr uniqueEnv = ptr->getEnv();
  JNIEnv *env = uniqueEnv.get();

  Local<Value> arg = info[0];
  Local<String> json = JSON::Stringify(info.Holder()->CreationContext(), arg).ToLocalChecked();

  String::Utf8Value jsonUTF8(info.GetIsolate(), json); // auto
  jstring jsonJString = env->NewStringUTF(*jsonUTF8); // new
  jstring resJString = static_cast<jstring>(env->CallObjectMethod(ptr->globalObjRef, v8EngineNative__call,
                                                                  jsonJString)); //new

  jboolean isCopy = false;
  const char *resCStr;
  if (resJString == nullptr) {
    // For whatever reason, this happens once at death time? Not sure why. I never return null.
    // Possibly an exception is thrown?
    resCStr = "{\"state\": \"noop\", \"value\": \"Got null from java \"}";
  } else {
    resCStr = env->GetStringUTFChars(resJString, &isCopy);
  } //new
//  printf("__call result: %s\n", resCStr);
//  fflush(stdout);
  Local<String> resLocalStr = String::NewFromUtf8(info.GetIsolate(), resCStr, NewStringType::kNormal).ToLocalChecked();

  if (resJString != nullptr) {
    env->ReleaseStringUTFChars(resJString, resCStr); //del
    env->DeleteLocalRef(resJString); // del
  }
  env->DeleteLocalRef(jsonJString); // del

  info.GetReturnValue().Set(resLocalStr);
}
