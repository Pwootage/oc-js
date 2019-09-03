#include <jni.h>
#include "com_pwootage_oc_js_duktape_DuktapeEngine.h"
#include "com_pwootage_oc_js_duktape_DuktapeStatic.h"
#include "DuktapeEngineNative.h"
#include <string>

using namespace std;


JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_duktape_DuktapeStatic_native_1init(JNIEnv *env, jclass clazz) {
  DukTapeEngineNative::Initialize(env, clazz);
}

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_duktape_DuktapeEngine_native_1start(JNIEnv *env, jobject self) {
  DukTapeEngineNative *engine = DukTapeEngineNative::getFromJava(env, self);
  if (engine == nullptr) {
    printf("Native start\n");
    fflush(stdout);

    engine = new DukTapeEngineNative(env, self);
    DukTapeEngineNative::setToJava(env, self, engine);
  }
}


JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_duktape_DuktapeEngine_native_1destroy(JNIEnv *env, jobject self) {
  DukTapeEngineNative *engine = DukTapeEngineNative::getFromJava(env, self);
  if (engine != nullptr) {
    printf("Native destroy\n");
    fflush(stdout);

    delete engine;
    DukTapeEngineNative::setToJava(env, self, nullptr);
  }
}

JNIEXPORT jstring JNICALL
Java_com_pwootage_oc_js_duktape_DuktapeEngine_native_1next(JNIEnv *env, jobject self, jstring next) {
  DukTapeEngineNative *engine = DukTapeEngineNative::getFromJava(env, self);
  if (engine != nullptr) {
    const char *utfChars = env->GetStringUTFChars(next, nullptr);
    string nextVal(utfChars);
    env->ReleaseStringUTFChars(next, utfChars);

//    DukTapeEngineNative::debug_print(u"Recieved next: " + nextVal);
    future<string> resFuture = engine->next(nextVal);
    auto status = resFuture.wait_for(chrono::seconds(1));
    if (status == future_status::ready) {
      string res = resFuture.get();
//      DukTapeEngineNative::debug_print(u"Recieved next result: " + res);
      return env->NewStringUTF(res.c_str());
    } else {
      return env->NewStringUTF(R"({"type":"unresponsive"})");
    }
  } else {
    return env->NewStringUTF(R"({"type":"no_engine"})");
  }
}
