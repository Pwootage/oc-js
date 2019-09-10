//
// Created by pwootage on 11/14/17.
//
#include "DuktapeEngineNative.h"

#include <cstdio>
#include <thread>
#include <functional>

using namespace std;

duk_bool_t duk_exec_timeout(void *udata) {
  return static_cast<DukTapeEngineNative*>(udata)->shouldKill;
}

DukTapeEngineNative::DukTapeEngineNative() {
  this->mainThread = new thread([this] { this->mainThreadFn(); });
}

DukTapeEngineNative::~DukTapeEngineNative() {
  // TODO: maybe a safer/cleaner thread kill
  debug_print("JS main thread kill");
  shouldKill = true;
  if (!isDead) {
    debug_print("JS main thread is not dead");
    //this->next(R"({"state": "error", "value": "kill"})");
    pthread_cancel(this->mainThread->native_handle());
  }
  debug_print("JS waiting for main thread");
  if (this->mainThread != nullptr) {
    this->mainThread->join();
    this->mainThread = nullptr;
  }
  duk_destroy_heap(this->context);
  this->context = nullptr;
  debug_print("JS main thread kill complete");
}

future<JSValuePtr> DukTapeEngineNative::next(JSValuePtr next) {
  future<JSValuePtr> res;
  {
    lock_guard<mutex> lock(this->executionMutex);

    if (this->deadResult) {
      promise<JSValuePtr> promise;
      promise.set_value(*this->deadResult);
      return promise.get_future();
    }

    this->nextInput = make_optional(next);
    this->outputPromise = make_optional(promise<JSValuePtr>());
    res = this->outputPromise->get_future();
  }
  this->engineWait.notify_one();
  return res;
}

void DukTapeEngineNative::mainThreadFn() {
  // Lock the engine
  this->engineLock = unique_lock(this->executionMutex);

  debug_print("JS main thread start");

  //init
  this->context = duk_create_heap(engine_alloc, engine_realloc, engine_free, this, engine_fatal);
  if (!this->context) {
    //TODO: Throw java exception
  }

  duk_push_global_stash(this->context);
  {
    duk_push_pointer(this->context, this);
    duk_put_prop_string(this->context, -2, "enginePtr");
  }
  duk_pop(this->context);

  duk_push_global_object(this->context);
  {
    duk_push_c_function(this->context, __yield, 1);
    duk_put_prop_string(this->context, -2, "__yield");

    duk_push_c_function(this->context, __compile, 2);
    duk_put_prop_string(this->context, -2, "__compile");

    duk_push_string(this->context, "global");
    duk_push_global_object(this->context);
    duk_def_prop(this->context, -3, DUK_DEFPROP_HAVE_VALUE);

    duk_del_prop_literal(this->context, -1, "Duktape");
  }
  duk_pop(this->context);

  // TODO; is there a CPU safety option here?
//  if (!JS_AddInterruptCallback(this->context, interruptCallback)) {
//    // TODO: throw java exception
//  }

  // First yield to get code to execute
  JSValuePtr src = this->yield(JSValuePtr(new JSNullValue()));
  if (src->getType() != JSValue::Type::STRING) {
    goto dead;
  }

  JSValuePtr res = this->compileAndExecute(src->asString()->value, "__bios__");

  dead:
  this->deadResult = make_optional(res);
  if (this->outputPromise.has_value()) {
    this->outputPromise->set_value(res);
  }
  // We're now dead and can't execute anymore. :'(
  this->isDead = true;

  debug_print("JS main thread end");

  this->engineLock.unlock();
}

JSValuePtr DukTapeEngineNative::yield(JSValuePtr output) {
  this->outputPromise->set_value(output);
  this->nextInput = nullopt;
  this->outputPromise = nullopt;
  this->engineWait.wait(this->engineLock, [this] {
    return this->nextInput.has_value();
  });
  fflush(stdout);
  return *this->nextInput;
}

JSValuePtr DukTapeEngineNative::compileAndExecute(const string& src, const string &filename) {
  duk_push_string(this->context, src.c_str());
  duk_push_string(this->context, filename.c_str());
  if (duk_pcompile(this->context, 0) != 0) {
    return "compile failed: " + string(duk_safe_to_string(this->context, -1));
  }
  if (duk_pcall(this->context, 0) != 0) {
    return "pcall failed: " + string(duk_safe_to_string(this->context, -1));
  }
  return string(duk_safe_to_string(this->context, -1));
}

duk_ret_t DukTapeEngineNative::__yield(duk_context *ctx) {
  // Grab the engine
  DukTapeEngineNative *native = nullptr;
  duk_push_global_stash(ctx);
  {
    duk_get_prop_string(ctx, -1, "enginePtr");
    native = static_cast<DukTapeEngineNative *>(duk_get_pointer(ctx, -1));
    duk_pop(ctx);
  }
  duk_pop(ctx);

  // pull out args
  string json(duk_require_string(ctx, 0));
  duk_pop(ctx);

  // yield
  string res = native->yield(json);

  // Return
  duk_push_string(ctx, res.c_str());
  return 1;
}

duk_ret_t DukTapeEngineNative::__compile(duk_context *ctx) {

  // pull out args
  string filenameVal(duk_require_string(ctx, 0));
  string srcVal(duk_require_string(ctx, 1));
  duk_pop(ctx);
  duk_pop(ctx);

  duk_push_string(ctx, srcVal.c_str());
  duk_push_string(ctx, filenameVal.c_str());
  if (duk_pcompile(ctx, 0) != 0) {
    return duk_throw(ctx);
  }

  return 1;
}

void DukTapeEngineNative::debug_print(const std::string &str) {
  printf("%s\n", str.c_str());
  fflush(stdout);
}

struct alloc_struct {
  uint32_t canary;
  size_t size;
};

constexpr uint32_t CANARY_VAL = 0xAABADDAD;

void *DukTapeEngineNative::engine_alloc(void *usrData, size_t size) {
  auto *native = static_cast<DukTapeEngineNative *>(usrData);

  size += sizeof(alloc_struct);

  if (native->allocatedMemory + size > native->maxMemory) {
    return nullptr;
  }

  native->allocatedMemory += size;
  alloc_struct *alloc = static_cast<alloc_struct *>(malloc(size));
  alloc->canary = CANARY_VAL;
  alloc->size = size;
  // Points at the end of the struct
  return alloc + 1;
}

void *DukTapeEngineNative::engine_realloc(void *usrData, void *ptr, size_t size) {
  auto *native = static_cast<DukTapeEngineNative *>(usrData);

  if (ptr == nullptr) {
    return engine_alloc(usrData, size);
  }

  auto *alloc = static_cast<alloc_struct *>(ptr) - 1;

  if (alloc->canary != CANARY_VAL) {
    debug_print("BAD REALLOC, WAS NOT ALLOCATED BY US");
    return nullptr;
  }

  size += sizeof(alloc_struct);

  if (size > alloc->size) {
    size_t diff = size - alloc->size;
    if (native->allocatedMemory + diff > native->maxMemory) {
      return nullptr;
    }
    native->allocatedMemory += diff;
  } else {
    size_t diff = alloc->size - size;
    native->allocatedMemory -= diff;
  }

  alloc = static_cast<alloc_struct *>(realloc(alloc, size));
  alloc->size = size;
  // Points at the end of the struct
  return alloc + 1;
}

void DukTapeEngineNative::engine_free(void *usrData, void *ptr) {
  auto *native = static_cast<DukTapeEngineNative *>(usrData);

  if (ptr == nullptr) {
    return;
  }

  auto *alloc = static_cast<alloc_struct *>(ptr) - 1;

  if (alloc->canary != CANARY_VAL) {
    debug_print("BAD FREE, WAS NOT ALLOCATED BY US");
    return;
  }

  if (native->allocatedMemory < alloc->size) {
    debug_print("BAD FREE, MEMORY WOULD GO NEGATIVE");
    native->allocatedMemory = 0;
  } else {
    native->allocatedMemory -= alloc->size;
  }

  alloc->canary = 0;
  free(alloc);
}

void DukTapeEngineNative::engine_fatal(void *usrData, const char *msg) {
  // Clean up and crash the vm
  auto *native = static_cast<DukTapeEngineNative *>(usrData);
  string smsg(msg);
  for (char &i : smsg) {
    // I mean, it's not exactly the most robust sanitization but it's better than nothin'
    if (i == '\\' || i == '"') {
      i = ' ';
    }
  }
  string deadResult = R"({"state": "error", "value": "kill: )" + smsg + R"("})";
  native->deadResult = make_optional(deadResult);
  if (native->outputPromise.has_value()) {
    native->outputPromise->set_value(deadResult);
  }
  // We're now dead and can't execute anymore. :'(
  native->isDead = true;

  debug_print("JS main thread end");

  native->engineLock.unlock();

  pthread_exit(nullptr);
}

size_t DukTapeEngineNative::getMaxMemory() const {
  return maxMemory;
}

void DukTapeEngineNative::setMaxMemory(size_t newMem) {
  maxMemory = newMem;
}

size_t DukTapeEngineNative::getAllocatedMemory() const {
  return allocatedMemory;
}
