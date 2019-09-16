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

JNIEXPORT jstring JNICALL
Java_com_pwootage_oc_js_spidermonkey_SpiderMonkeyEngine_native_1next(JNIEnv *env, jobject self, jstring next) {
  SpiderMonkeyEngineNative *engine = getSpiderMonkeyFromJava(env, self);
  if (engine != nullptr) {
    const jchar *utfChars = env->GetStringChars(next, nullptr);
    // jchar* is compatible with char16, just is unsigned
    u16string nextVal(reinterpret_cast<const char16_t *>(utfChars));
    env->ReleaseStringChars(next, utfChars);

//    SpiderMonkeyEngineNative::debug_print(u"Recieved next: " + nextVal);
    future<u16string> resFuture = engine->next(nextVal);
    auto status = resFuture.wait_for(chrono::seconds(1));
    if (status == future_status::ready) {
      u16string res = resFuture.get();
//      SpiderMonkeyEngineNative::debug_print(u"Recieved next result: " + res);
      // Again, jchar is compatible
      return env->NewString(reinterpret_cast<const jchar *>(res.c_str()), res.length());
    } else {
      return env->NewStringUTF(R"({"type":"unresponsive"})");
    }
  } else {
    return env->NewStringUTF(R"({"type":"no_engine"})");
  }
}

void InitializeSpiderMonkey(JNIEnv *env, jobject clazz) {
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