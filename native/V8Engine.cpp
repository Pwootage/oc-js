#include "com_pwootage_oc_js_v8_V8Engine.h"
#include "com_pwootage_oc_js_v8_V8Static.h"
#include "include/v8.h"
#include "include/libplatform/libplatform.h"

using namespace v8;

static Platform *v8Platform;

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Static_native_1init(JNIEnv *env, jclass clazz){
    jobject icudtl = env->GetStaticObjectField(clazz, env->GetStaticFieldID(clazz, "icudtl", "Ljava/nio/ByteBuffer"));
    jobject natives_blob = env->GetStaticObjectField(clazz, env->GetStaticFieldID(clazz, "natives_blob", "Ljava/nio/ByteBuffer"));
    jobject snapshot_blob = env->GetStaticObjectField(clazz, env->GetStaticFieldID(clazz, "snapshot_blob", "Ljava/nio/ByteBuffer"));


    StartupData icudtl_data{
            reinterpret_cast<const char *>(env->GetDirectBufferAddress(icudtl)),
            static_cast<int>(env->GetDirectBufferCapacity(icudtl))
    };
    StartupData natives_blob_data{
            reinterpret_cast<const char *>(env->GetDirectBufferAddress(natives_blob)),
            static_cast<int>(env->GetDirectBufferCapacity(natives_blob))
    };
    StartupData snapshot_blob_data{
            reinterpret_cast<const char *>(env->GetDirectBufferAddress(snapshot_blob)),
            static_cast<int>(env->GetDirectBufferCapacity(snapshot_blob))
    };
    V8::SetNativesDataBlob(&natives_blob_data);
    V8::SetSnapshotDataBlob(&snapshot_blob_data);
    //TODO: icudtl?
//    V8::Initi
    v8Platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(v8Platform);
    V8::Initialize();
}

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_native_1start(JNIEnv *env, jobject self) {

}


JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_native_1destroy(JNIEnv *env, jobject self) {

}
