//
// Created by pwootage on 11/14/17.
//

#ifndef OCJS_V8ENGINENATIVE_H
#define OCJS_V8ENGINENATIVE_H
#include <jni.h>
#include "include/v8.h"
#include "include/libplatform/libplatform.h"

/**
 * The native equivalent of V8Engine, where all the V8-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class V8EngineNative {
public:
    V8EngineNative();
    ~V8EngineNative();

    static void Initialize(JNIEnv *env, jclass clazz);
    static V8EngineNative *getFromJava(JNIEnv *env, jobject obj);
private:
};


#endif //OCJS_V8ENGINENATIVE_H
