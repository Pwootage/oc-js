//
// Created by pwootage on 11/14/17.
//

#ifndef OCJS_SPIDERMONKEYENGINENATIVE_H
#define OCJS_SPIDERMONKEYENGINENATIVE_H

#include <jni.h>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <string>
#include <mutex>
#include <optional>
#include <jsapi.h>
//#include "include/v8.h"
//#include "include/libplatform/libplatform.h"

/**
 * The native equivalent of SpiderMonkeyEngine, where all the SpiderMonkey-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class SpiderMonkeyEngineNative {
public:
  using JNIPtr = std::unique_ptr<JNIEnv, std::function<void(JNIEnv *)>>;

  SpiderMonkeyEngineNative(JNIEnv *env, jobject obj);
  ~SpiderMonkeyEngineNative();

  static void Initialize(JNIEnv *env, jclass clazz);
  static SpiderMonkeyEngineNative *getFromJava(JNIEnv *env, jobject obj);
  static void setToJava(JNIEnv *env, jobject obj, SpiderMonkeyEngineNative *data);

  std::thread mainThread;
  JSContext *getContext();

  std::future<std::u16string> next(std::u16string next);

  static constexpr size_t MAX_STR_SIZE = 1024 * 1024;
private:
  // JVM Stuff
  JavaVM *javaVM{nullptr};
  jobject globalObjRef;

  JNIPtr getEnv();

  // JS context stuff
  JSContext *context;
  JS::RootedObject *globalObject;

  //JS engine stuff
  std::u16string compileAndExecute(std::u16string src, std::u16string filename);
  //  JS::RootedObject convertException();

  // Thread stuff
  void mainThreadFn();
  std::u16string yield(const std::u16string& output);

  std::mutex executionMutex;
  std::condition_variable engineWait;
  std::unique_lock<std::mutex> engineLock;
  std::optional<std::u16string> nextInput = std::nullopt;
  std::optional<std::promise<std::u16string>> outputPromise = std::make_optional(std::promise<std::u16string>());
  std::optional<std::u16string> deadResult = std::nullopt;

  bool shouldKill{false};
  bool isDead{false};

  // js callbacks

  // To java (eventually)
  static bool __yield(JSContext *ctx, unsigned argc, JS::Value *vp);
  static bool __compile(JSContext *ctx, unsigned argc, JS::Value *vp);

  // Safety
  static bool interruptCallback(JSContext *ctx);

  // Convenience
  static std::u16string getU16String(JSContext *ctx, const JS::HandleValue &handle);
  static std::string getU8String(JSContext *ctx, const JS::HandleValue &handle);
};


#endif //OCJS_V8ENGINENATIVE_H
