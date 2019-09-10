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
#include "JSValue.hpp"

/**
 * The native equivalent of DukTapeEngine, where all the DukTape-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class DukTapeEngineNative {
public:
  DukTapeEngineNative();
  ~DukTapeEngineNative();

  std::thread *mainThread;

  std::future<JSValuePtr> next(JSValuePtr next);

  static constexpr size_t MAX_STR_SIZE = 1024 * 1024;

  static void debug_print(const std::string& str);

  [[nodiscard]] size_t getMaxMemory() const;
  void setMaxMemory(size_t maxMemory);
  [[nodiscard]] size_t getAllocatedMemory() const;


private:
  // memory
  size_t maxMemory = 16 * 1024 * 1024;
  size_t allocatedMemory = 0;

  // JS context stuff
  duk_context *context = nullptr;

  //JS engine stuff
  JSValuePtr compileAndExecute(const std::string& src, const std::string &filename);
  //  JS::RootedObject convertException();

  // Thread stuff
  void mainThreadFn();
  JSValuePtr yield(JSValuePtr output);

  std::mutex executionMutex;
  std::condition_variable engineWait;
  std::unique_lock<std::mutex> engineLock;
  std::optional<JSValuePtr> nextInput = std::nullopt;
  std::optional<std::promise<JSValuePtr>> outputPromise = std::make_optional(std::promise<JSValuePtr>());
  std::optional<JSValuePtr> deadResult = std::nullopt;

  bool shouldKill{false};
  bool isDead{false};

  // js callbacks

  // To java (eventually)
  static duk_ret_t __yield(duk_context *ctx);
  static duk_ret_t __compile(duk_context *ctx);

  // memory functions
  static void* engine_alloc(void* usrData, size_t size);
  static void* engine_realloc(void* usrData, void *ptr, size_t size);
  static void engine_free(void *usrData, void *ptr);
  static void engine_fatal(void *usrData, const char *msg);

  friend duk_bool_t duk_exec_timeout(void *udata);
};


#endif //OCJS_V8ENGINENATIVE_H
