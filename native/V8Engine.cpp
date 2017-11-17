#include <jni.h>
#include "com_pwootage_oc_js_v8_V8Engine.h"
#include "com_pwootage_oc_js_v8_V8Static.h"
#include "V8EngineNative.h"

using namespace v8;

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Static_native_1init(JNIEnv *env, jclass clazz) {
    V8EngineNative::Initialize(env, clazz);
}

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_native_1start(JNIEnv *env, jobject self) {
    V8EngineNative *engine = V8EngineNative::getFromJava(env, self);
    if (engine == nullptr) {
        printf("Native start\n");
        fflush(stdout);

        engine = new V8EngineNative(env, self);
        V8EngineNative::setToJava(env, self, engine);
    }
}


JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_native_1destroy(JNIEnv *env, jobject self) {
    V8EngineNative *engine = V8EngineNative::getFromJava(env, self);
    if (engine != nullptr) {
        printf("Native destroy\n");
        fflush(stdout);

        delete engine;
        V8EngineNative::setToJava(env, self, nullptr);
    }
}

JNIEXPORT jstring JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_compile_1and_1execute(JNIEnv *env, jobject self, jstring src, jstring fname, jint execContext) {
  V8EngineNative *engine = V8EngineNative::getFromJava(env, self);
  if (engine != nullptr) {
    V8EngineNative::ExecutionContext ctx = V8EngineNative::ExecutionContext::user;
    if (execContext == 0) {
      ctx = V8EngineNative::ExecutionContext::bios;
    } else if (execContext == 1) {
      ctx = V8EngineNative::ExecutionContext::kernel;
    }

    HandleScope handle_scope(engine->getIsolate());
    Local<Context> context = Context::New(engine->getIsolate());
    Context::Scope context_scope(context);
    Local<Value> res = engine->compileAndExecute(src, fname, ctx);
    Local<String> jsonStr = JSON::Stringify(context, res).ToLocalChecked();
    String::Utf8Value resStr(engine->getIsolate(), jsonStr);
    return env->NewStringUTF(*resStr);
  }
  return env->NewStringUTF("{\"state\": \"error\", \"result\": \"No V8 exists!\"}");
}
