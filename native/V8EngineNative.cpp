//
// Created by pwootage on 11/14/17.
//
#include "V8EngineNative.h"
#include <cstdio>
#include <cstring>

using namespace v8;

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


//    StartupData icudtl_data{
//            reinterpret_cast<const char *>(env->GetDirectBufferAddress(icudtl)),
//            static_cast<int>(env->GetDirectBufferCapacity(icudtl))
//    };
    icudtl_data = getData(env, icudtl);
    natives_blob_data = getData(env, natives_blob);
    snapshot_blob_data = getData(env, snapshot_blob);

    V8::SetNativesDataBlob(natives_blob_data);
    V8::SetSnapshotDataBlob(snapshot_blob_data);
//    V8::InitializeICUDefaultLocation("/home/pwootage/projects/oc-js/src/main/resources/assets/oc-js/v8/");
//    V8::InitializeExternalStartupData("/home/pwootage/projects/oc-js/src/main/resources/assets/oc-js/v8/");
    v8Platform = platform::CreateDefaultPlatform(1,
                                                 platform::IdleTaskSupport::kDisabled,
                                                 platform::InProcessStackDumping::kDisabled);
    V8::InitializePlatform(v8Platform);
    V8::Initialize();
//
    jclass v8EngineClass = env->FindClass("com/pwootage/oc/js/v8/V8Engine");
    v8EngineNativeFID = env->GetFieldID(v8EngineClass, "v8EngineNative", "J");
}

V8EngineNative *V8EngineNative::getFromJava(JNIEnv *env, jobject obj) {
    return reinterpret_cast<V8EngineNative *>(env->GetLongField(obj, v8EngineNativeFID));
}


V8EngineNative::V8EngineNative() {

}

V8EngineNative::~V8EngineNative() {

}
