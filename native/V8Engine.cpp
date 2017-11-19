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
Java_com_pwootage_oc_js_v8_V8Engine_compile_1and_1execute(JNIEnv *env, jobject self, jstring src, jstring fname) {
  V8EngineNative *engine = V8EngineNative::getFromJava(env, self);
  if (engine != nullptr) {
    Locker locker(engine->getIsolate());
    Isolate::Scope isolateScope(engine->getIsolate());
    HandleScope handle_scope(engine->getIsolate());
    Local<String> res = engine->compileAndExecute(src, fname);

    if (!res.IsEmpty()) {
      String::Utf8Value resStr(engine->getIsolate(), res);
      return env->NewStringUTF(*resStr);
    } else {
      return env->NewStringUTF("{\"state\": \"error\", \"message\": \"Got no res!\"}");
    }

  }
  return env->NewStringUTF("{\"state\": \"error\", \"message\": \"No V8 exists!\"}");
}
