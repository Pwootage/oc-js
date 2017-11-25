#include <jni.h>
#include "com_pwootage_oc_js_v8_V8Engine.h"
#include "com_pwootage_oc_js_v8_V8Static.h"
#include "V8EngineNative.h"
#include <string>

using namespace v8;
using namespace std;

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
Java_com_pwootage_oc_js_v8_V8Engine_native_1next(JNIEnv *env, jobject self, jstring next) {
  V8EngineNative *engine = V8EngineNative::getFromJava(env, self);
  if (engine != nullptr) {
    const char* utfChars = env->GetStringUTFChars(next, nullptr);
    string nextVal(utfChars);
    env->ReleaseStringUTFChars(next, utfChars);

    future<string> resFuture = engine->next(nextVal);
    auto status = resFuture.wait_for(chrono::seconds(1));
    if (status == future_status::ready) {
      string res = resFuture.get();
      return env->NewStringUTF(res.c_str());
    } else {
      return env->NewStringUTF("{\"type\":\"unresponsive\"}");
    }
  } else {
    return env->NewStringUTF("{\"type\":\"no_engine\"}");
  }
}
