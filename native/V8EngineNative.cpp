//
// Created by pwootage on 11/14/17.
//
#include "V8EngineNative.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

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

  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate = Isolate::New(create_params);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    //TODO: attach global stuff to this template
    Local<ObjectTemplate> biosGlobal = ObjectTemplate::New(isolate);
    biosContext.Reset(isolate, Context::New(isolate, nullptr, biosGlobal));
    kernelContext.Reset(isolate, Context::New(isolate));
    userContext.Reset(isolate, Context::New(isolate));
  }
}

V8EngineNative::~V8EngineNative() {
  isolate->TerminateExecution();
  while (isolate->IsExecutionTerminating()) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }
  biosContext.Reset();
  kernelContext.Reset();
  userContext.Reset();
  isolate->Dispose();
  delete create_params.array_buffer_allocator;

}

unique_ptr<JNIEnv, function<void(JNIEnv *)>> V8EngineNative::getEnv() {
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

  std::unique_ptr<JNIEnv, function<void(JNIEnv *)>> res(env, [this](JNIEnv *ptr) {
//      if (detach) {
      this->javaVM->DetachCurrentThread();
//      }
  });
  return res;
}

v8::Local<v8::Value>
V8EngineNative::compileAndExecute(jstring src, jstring filename, ExecutionContext executionContext) {
  auto env = getEnv();

  Isolate::Scope isolate_scope(isolate);
  EscapableHandleScope handle_scope(isolate);
  const char *srcCStr = env->GetStringUTFChars(src, JNI_FALSE);
  Local<String> srcStr = String::NewFromUtf8(isolate, srcCStr, NewStringType::kNormal).ToLocalChecked();
  env->ReleaseStringUTFChars(src, srcCStr);

  const char *filenameCStr = env->GetStringUTFChars(filename, JNI_FALSE);
  Local<String> filenameStr = String::NewFromUtf8(isolate, filenameCStr, NewStringType::kNormal).ToLocalChecked();
  env->ReleaseStringUTFChars(filename, filenameCStr);

  Local<Context> context;
  switch (executionContext) {
    case ExecutionContext::bios:
      context = biosContext.Get(isolate);
      break;
    case ExecutionContext::kernel:
      context = kernelContext.Get(isolate);
      break;
    case ExecutionContext::user:
      context = userContext.Get(isolate);
      break;
  }

  Context::Scope contextScope(context);

  TryCatch try_catch(isolate);
  Local<Script> script = Script::Compile(srcStr, filenameStr);
  if (script.IsEmpty()) {
    return handle_scope.Escape(convertException(context, try_catch));
  }

  Local<Value> scriptResult = script->Run();
  if (scriptResult.IsEmpty()) {
    return handle_scope.Escape(convertException(context, try_catch));
  }

  Local<Object> res = Object::New(isolate);
  res->Set(
      String::NewFromUtf8(isolate, "state", NewStringType::kNormal).ToLocalChecked(),
      String::NewFromUtf8(isolate, "success", NewStringType::kNormal).ToLocalChecked()
  );
  res->Set(
      String::NewFromUtf8(isolate, "result", NewStringType::kNormal).ToLocalChecked(),
      scriptResult
  );
  return handle_scope.Escape(res);
}

Isolate *V8EngineNative::getIsolate() {
  return isolate;
}

Local<Value> V8EngineNative::convertException(Local<Context> &context, TryCatch &tryCatch) {
  EscapableHandleScope handle_scope(isolate);
  Local<Message> message = tryCatch.Message();
  if (message.IsEmpty()) {
    return handle_scope.Escape(Local<Value>());
  }

  Local<Object> result = Object::New(isolate);
  result->Set(
      String::NewFromUtf8(isolate, "state", NewStringType::kNormal).ToLocalChecked(),
      String::NewFromUtf8(isolate, "error", NewStringType::kNormal).ToLocalChecked()
  );
  result->Set(
      String::NewFromUtf8(isolate, "file", NewStringType::kNormal).ToLocalChecked(),
      message->GetScriptResourceName()
  );
  result->Set(
      String::NewFromUtf8(isolate, "src", NewStringType::kNormal).ToLocalChecked(),
      message->GetSourceLine()
  );
  Maybe<int> line = message->GetLineNumber(context);
  if (line.IsJust()) {
    result->Set(
        String::NewFromUtf8(isolate, "line", NewStringType::kNormal).ToLocalChecked(),
        Integer::New(isolate, line.FromJust())
    );
  }
  Maybe<int> startCol = message->GetStartColumn(context);
  if (startCol.IsJust()) {
    result->Set(
        String::NewFromUtf8(isolate, "start", NewStringType::kNormal).ToLocalChecked(),
        Integer::New(isolate, startCol.FromJust())
    );
  }
  Maybe<int> endCol = message->GetStartColumn(context);
  if (endCol.IsJust()) {
    result->Set(
        String::NewFromUtf8(isolate, "end", NewStringType::kNormal).ToLocalChecked(),
        Integer::New(isolate, endCol.FromJust())
    );
  }

  Local<StackTrace> stackTrace = message->GetStackTrace();
  if (!stackTrace.IsEmpty()) {
    Local<Array> stackArray = Array::New(isolate, stackTrace->GetFrameCount());
    for (uint32_t i = 0; i < stackTrace->GetFrameCount(); i++) {
      Local<StackFrame> frame = stackTrace->GetFrame(i);
      Local<Object> resFrame = Object::New(isolate);
      resFrame->Set(
          String::NewFromUtf8(isolate, "file", NewStringType::kNormal).ToLocalChecked(),
          frame->GetScriptName()
      );
      resFrame->Set(
          String::NewFromUtf8(isolate, "function", NewStringType::kNormal).ToLocalChecked(),
          frame->GetFunctionName()
      );
      resFrame->Set(
          String::NewFromUtf8(isolate, "line", NewStringType::kNormal).ToLocalChecked(),
          Integer::New(isolate, frame->GetLineNumber())
      );
      resFrame->Set(
          String::NewFromUtf8(isolate, "col", NewStringType::kNormal).ToLocalChecked(),
          Integer::New(isolate, frame->GetColumn())
      );
      stackArray->Set(i, resFrame);
    }

    result->Set(
        String::NewFromUtf8(isolate, "stacktrace", NewStringType::kNormal).ToLocalChecked(),
        stackArray
    );
  }

  result->Set(
      String::NewFromUtf8(isolate, "origin", NewStringType::kNormal).ToLocalChecked(),
      message->GetScriptOrigin().ResourceName()
  );
  result->Set(
      String::NewFromUtf8(isolate, "message", NewStringType::kNormal).ToLocalChecked(),
      message->Get()
  );
  return handle_scope.Escape(result);
}
