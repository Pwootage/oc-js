//
// Created by pwootage on 11/14/17.
//

#ifndef OCJS_V8ENGINENATIVE_H
#define OCJS_V8ENGINENATIVE_H
#define V8_DEPRECATION_WARNINGS
#define V8_IMMINENT_DEPRECATION_WARNINGS
#include <jni.h>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <string>
#include <mutex>
#include <optional>
#include "include/v8.h"
#include "include/libplatform/libplatform.h"

/**
 * The native equivalent of V8Engine, where all the V8-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class V8EngineNative {
public:
    using JNIPtr = std::unique_ptr<JNIEnv, std::function<void(JNIEnv *)>>;

    V8EngineNative(JNIEnv *env, jobject obj);
    ~V8EngineNative();

    static void Initialize(JNIEnv *env, jclass clazz);
    static V8EngineNative *getFromJava(JNIEnv *env, jobject obj);
    static void setToJava(JNIEnv *env, jobject obj, V8EngineNative *data);

    std::thread mainThread;
    v8::Isolate *getIsolate();
    std::future<std::string> next(std::string next);

    v8::Global<v8::Context> contextRef;
private:
    JavaVM *javaVM;
    jobject globalObjRef;

    v8::Isolate::CreateParams create_params;
    v8::Isolate *isolate;

    JNIPtr getEnv();
    v8::Local<v8::Object> convertException(v8::Local<v8::Context> context, v8::TryCatch &tryCatch);
    std::string compileAndExecute(std::string src, std::string filename);

    // Thread stuff
    void mainThreadFn();
    std::string yield(std::string output);

    std::mutex executionMutex;
    std::condition_variable engineWait;
    std::unique_lock<std::mutex> engineLock;
    std::optional<std::string> nextInput = std::nullopt;
    std::optional<std::promise<std::string>> outputPromise = std::make_optional(std::promise<std::string>());
    std::optional<std::string> deadResult = std::nullopt;

    /** To java (eventually) */
    static void __yield(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void __compile(const v8::FunctionCallbackInfo<v8::Value> &info);
};


#endif //OCJS_V8ENGINENATIVE_H
