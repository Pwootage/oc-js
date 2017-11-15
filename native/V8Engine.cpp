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

        engine = new V8EngineNative();
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
