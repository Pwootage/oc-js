#include <jni.h>
#include "com_pwootage_oc_js_spidermonkey_SpiderMonkeyEngine.h"
#include "com_pwootage_oc_js_spidermonkey_SpiderMonkeyStatic.h"
#include "SpiderMonkeyEngineNative.h"
#include <string>
#include <js/Initialization.h>

using namespace std;

void InitializeSpiderMonkey(JNIEnv *env, jobject clazz);
SpiderMonkeyEngineNative *getSpiderMonkeyFromJava(JNIEnv *env, jobject obj);
void setSpiderMonkeyToJava(JNIEnv *env, jobject obj, SpiderMonkeyEngineNative *data);

jfieldID spiderMonkeyEngineNativeFID = nullptr;

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_spidermonkey_SpiderMonkeyStatic_native_1init(JNIEnv *env, jobject self) {
  InitializeSpiderMonkey(env, self);
}

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_spidermonkey_SpiderMonkeyEngine_native_1start(JNIEnv *env, jobject self) {
  SpiderMonkeyEngineNative *engine = getSpiderMonkeyFromJava(env, self);
  if (engine == nullptr) {
    printf("Native start\n");
    fflush(stdout);

    engine = new SpiderMonkeyEngineNative();
    setSpiderMonkeyToJava(env, self, engine);
  }
}


JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_spidermonkey_SpiderMonkeyEngine_native_1destroy(JNIEnv *env, jobject self) {
  SpiderMonkeyEngineNative *engine = getSpiderMonkeyFromJava(env, self);
  if (engine != nullptr) {
    printf("Native destroy\n");
    fflush(stdout);

    delete engine;
    setSpiderMonkeyToJava(env, self, nullptr);
  }
}

JNIEXPORT jobject JNICALL
Java_com_pwootage_oc_js_spidermonkey_SpiderMonkeyEngine_native_1next(JNIEnv *env, jobject self, jobject next) {
  SpiderMonkeyEngineNative *engine = getSpiderMonkeyFromJava(env, self);
  if (engine != nullptr) {
    auto value = OCJS::JSValue::fromJVM(env, next);
    future<OCJS::JSValuePtr> resFuture = engine->next(value);
    auto status = resFuture.wait_for(chrono::seconds(1));
    if (status == future_status::ready) {
      OCJS::JSValuePtr res = resFuture.get();
      return res->toJVM(env);
    } else {
      return env->NewStringUTF(R"({"type":"unresponsive"})");
    }
  } else {
    return env->NewStringUTF(R"({"type":"no_engine"})");
  }
}

void InitializeSpiderMonkey(JNIEnv *env, jobject clazz) {
  OCJS::JSValue::jvmInit(env);
  JS_Init();

  jclass v8EngineClass = env->FindClass("com/pwootage/oc/js/spidermonkey/SpiderMonkeyEngine");
  spiderMonkeyEngineNativeFID = env->GetFieldID(v8EngineClass, "spiderMonkeyEngineNative", "J");
}

SpiderMonkeyEngineNative *getSpiderMonkeyFromJava(JNIEnv *env, jobject obj) {
  return reinterpret_cast<SpiderMonkeyEngineNative *>(env->GetLongField(obj, spiderMonkeyEngineNativeFID));
}

void setSpiderMonkeyToJava(JNIEnv *env, jobject obj, SpiderMonkeyEngineNative *data) {
  env->SetLongField(obj, spiderMonkeyEngineNativeFID, reinterpret_cast<jlong>(data));
}