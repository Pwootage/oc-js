//
// Created by pwootage on 11/14/17.
//

#ifndef OCJS_DukTapeEngineNativeNATIVE_H
#define OCJS_DukTapeEngineNativeNATIVE_H

#include <jni.h>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <string>
#include <mutex>
#include <optional>
#include "duktape.h"

/**
 * The native equivalent of SpiderMonkeyEngine, where all the SpiderMonkey-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class DukTapeEngineNative {
public:
  using JNIPtr = std::unique_ptr<JNIEnv, std::function<void(JNIEnv *)>>;

  DukTapeEngineNative(JNIEnv *env, jobject obj);
  ~DukTapeEngineNative();

  static void Initialize(JNIEnv *env, jclass clazz);
  static DukTapeEngineNative *getFromJava(JNIEnv *env, jobject obj);
  static void setToJava(JNIEnv *env, jobject obj, DukTapeEngineNative *data);

  std::thread *mainThread;

  std::future<std::string> next(std::string next);

  static constexpr size_t MAX_STR_SIZE = 1024 * 1024;

  static void debug_print(const std::string& str);
private:
  // JVM Stuff
  JavaVM *javaVM{nullptr};
  jobject globalObjRef;

  JNIPtr getEnv();

  // JS context stuff
  duk_context *context = nullptr;

  //JS engine stuff
  std::string compileAndExecute(std::string src, std::string filename);
  //  JS::RootedObject convertException();

  // Thread stuff
  void mainThreadFn();
  std::string yield(const std::string& output);

  std::mutex executionMutex;
  std::condition_variable engineWait;
  std::unique_lock<std::mutex> engineLock;
  std::optional<std::string> nextInput = std::nullopt;
  std::optional<std::promise<std::string>> outputPromise = std::make_optional(std::promise<std::string>());
  std::optional<std::string> deadResult = std::nullopt;

  bool shouldKill{false};
  bool isDead{false};

  // js callbacks

  // To java (eventually)
  static duk_ret_t __yield(duk_context *ctx);
  static duk_ret_t __compile(duk_context *ctx);
};


#endif //OCJS_V8ENGINENATIVE_H
