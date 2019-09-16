//
// Created by pwootage on 11/14/17.
//

#ifndef OCJS_DukTapeEngineNativeNATIVE_H
#define OCJS_DukTapeEngineNativeNATIVE_H

#include "../JSEngine.hpp"
#include "duktape.h"

/**
 * The native equivalent of DukTapeEngine, where all the DukTape-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class DukTapeEngineNative : public JSEngine {
public:
  DukTapeEngineNative();
  ~DukTapeEngineNative() override;

  std::future<OCJS::JSValuePtr> next(OCJS::JSValuePtr next) override;
  [[nodiscard]] size_t getMaxMemory() const override;
  void setMaxMemory(size_t maxMemory) override;
  [[nodiscard]] size_t getAllocatedMemory() const override;
private:
  // memory
  size_t maxMemory = 16 * 1024 * 1024;
  size_t allocatedMemory = 0;

  // JS context stuff
  duk_context *context = nullptr;

  //JS engine stuff
  OCJS::JSValuePtr compileAndExecute(const std::string& src, const std::string &filename);
  void pushJSValue(const OCJS::JSValuePtr& ptr);
  OCJS::JSValuePtr convertObjectToJSValue(duk_idx_t idx);

  // Thread stuff
  std::thread mainThread;
  void mainThreadFn();
  OCJS::JSValuePtr yield(const OCJS::JSValuePtr& output);

  std::mutex executionMutex;
  std::condition_variable engineWait;
  std::unique_lock<std::mutex> engineLock;
  std::optional<OCJS::JSValuePtr> nextInput = std::nullopt;
  std::optional<std::promise<OCJS::JSValuePtr>> outputPromise = std::make_optional(std::promise<OCJS::JSValuePtr>());
  std::optional<OCJS::JSValuePtr> deadResult = std::nullopt;

  bool shouldKill{false};
  bool isDead{false};

  // js callbacks
  static duk_ret_t __yield(duk_context *ctx);
  static duk_ret_t __compile(duk_context *ctx);

  // duk functions
  static void* engine_alloc(void* usrData, size_t size);
  static void* engine_realloc(void* usrData, void *ptr, size_t size);
  static void engine_free(void *usrData, void *ptr);
  static void engine_fatal(void *usrData, const char *msg);
  friend duk_bool_t duk_exec_timeout(void *udata);

  // utils
  static void debug_print(const std::string& str);
};


#endif //OCJS_V8ENGINENATIVE_H
