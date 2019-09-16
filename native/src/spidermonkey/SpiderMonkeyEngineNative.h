//
// Created by pwootage on 11/14/17.
//

#ifndef OCJS_SPIDERMONKEYENGINENATIVE_H
#define OCJS_SPIDERMONKEYENGINENATIVE_H

#include "../JSEngine.hpp"
#include <jsapi.h>

/**
 * The native equivalent of SpiderMonkeyEngine, where all the SpiderMonkey-related classes are stored.
 *
 * We could store these as a bunch of 'long' in the Java class, but that also leads to issues
 * */
class SpiderMonkeyEngineNative : public JSEngine {
public:
  SpiderMonkeyEngineNative();
  ~SpiderMonkeyEngineNative() override;
  std::future<OCJS::JSValuePtr> next(OCJS::JSValuePtr next) override;
  static constexpr size_t MAX_STR_SIZE = 1024 * 1024;
  size_t getMaxMemory() const override;
  void setMaxMemory(size_t maxMemory) override;
  size_t getAllocatedMemory() const override;
private:
  // JS context stuff
  JSContext *context = nullptr;
  JS::RootedObject *globalObject = nullptr;

  //JS engine stuff
  OCJS::JSValuePtr compileAndExecute(const std::u16string &src, const std::u16string &filename);
  bool getJSValue(const OCJS::JSValuePtr &ptr, JS::MutableHandleValue vp);
  OCJS::JSValuePtr convertObjectToJSValue(const JS::HandleValue &val);

  // Thread stuff
  std::thread mainThread;
  void mainThreadFn();
  OCJS::JSValuePtr yield(const OCJS::JSValuePtr &output);

  std::mutex executionMutex;
  std::condition_variable engineWait;
  std::unique_lock<std::mutex> engineLock;
  std::optional<OCJS::JSValuePtr> nextInput = std::nullopt;
  std::optional<std::promise<OCJS::JSValuePtr>> outputPromise = std::make_optional(std::promise<OCJS::JSValuePtr>());
  std::optional<OCJS::JSValuePtr> deadResult = std::nullopt;

  bool shouldKill{false};
  bool isDead{false};

  // js callbacks

  // To java (eventually)
  static bool __yield(JSContext *ctx, unsigned argc, JS::Value *vp);
  static bool __compile(JSContext *ctx, unsigned argc, JS::Value *vp);

  // Safety
  static bool interruptCallback(JSContext *ctx);

  // Convenience
  static void debug_print(const std::u16string& str);
  static std::u16string getU16String(JSContext *ctx, const JS::HandleValue &handle);
  static std::string getU8String(JSContext *ctx, const JS::HandleValue &handle);
};


#endif //OCJS_V8ENGINENATIVE_H
