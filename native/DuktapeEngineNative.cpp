//
// Created by pwootage on 11/14/17.
//
#include "DuktapeEngineNative.h"

#include <cstdio>
#include <iostream>
#include <thread>
#include <functional>

using namespace std;

jfieldID DukTapeEngineNativeFID = nullptr;

void DukTapeEngineNative::Initialize(JNIEnv *env, jclass clazz) {
  jclass v8EngineClass = env->FindClass("com/pwootage/oc/js/duktape/DuktapeEngine");
  DukTapeEngineNativeFID = env->GetFieldID(v8EngineClass, "duktapeEngineNative", "J");
}

DukTapeEngineNative *DukTapeEngineNative::getFromJava(JNIEnv *env, jobject obj) {
  return reinterpret_cast<DukTapeEngineNative *>(env->GetLongField(obj, DukTapeEngineNativeFID));
}

void DukTapeEngineNative::setToJava(JNIEnv *env, jobject obj, DukTapeEngineNative *data) {
  env->SetLongField(obj, DukTapeEngineNativeFID, reinterpret_cast<jlong>(data));
}

DukTapeEngineNative::DukTapeEngineNative(JNIEnv *env, jobject obj) {
  env->GetJavaVM(&javaVM);
  globalObjRef = env->NewGlobalRef(obj);

  this->mainThread = new thread([this] { this->mainThreadFn(); });
}

DukTapeEngineNative::~DukTapeEngineNative() {
  // TODO: maybe a safer/cleaner thread kill
  debug_print("JS main thread kill");
  shouldKill = true;
  if (!isDead) {
    debug_print("JS main thread is not dead");
    this->next(R"({"state": "error", "value": "kill"})");
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

DukTapeEngineNative::JNIPtr DukTapeEngineNative::getEnv() {
  bool detach = false;
  JNIEnv *env = nullptr;
  // double check it's all ok
  int getEnvStat = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
  if (getEnvStat == JNI_EDETACHED) {
    detach = true;
    if (javaVM->AttachCurrentThread((void **) &env, NULL) != 0) {
      cout << "Failed to attach" << endl;
    }
  } else if (getEnvStat == JNI_OK) {
    detach = false;
  }

  return JNIPtr(env, [this](JNIEnv *ptr) {
//      if (detach) {
    this->javaVM->DetachCurrentThread();
//      }
  });
}

future<string> DukTapeEngineNative::next(string next) {
  future<string> res;
  {
    lock_guard<mutex> lock(this->executionMutex);

    if (this->deadResult) {
      promise<string> promise;
      promise.set_value(*this->deadResult);
      return promise.get_future();
    }

    this->nextInput = make_optional(next);
    this->outputPromise = make_optional(promise<string>());
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
  // TODO: memory management
  this->context = duk_create_heap_default();
  if (this->context) {
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
  }
  duk_pop(this->context);

  // TODO; is there a CPU safety option here?
//  if (!JS_AddInterruptCallback(this->context, interruptCallback)) {
//    // TODO: throw java exception
//  }

  // First yield to get code to execute
  string src = this->yield(R"({"type": "__bios__"})");
  string res = this->compileAndExecute(src, "__bios__");
  this->deadResult = make_optional(res);
  if (this->outputPromise.has_value()) {
    this->outputPromise->set_value(res);
  }
  // We're now dead and can't execute anymore. :'(
  this->isDead = true;

  debug_print("JS main thread end");

  this->engineLock.unlock();
}

string DukTapeEngineNative::yield(const string &output) {
  this->outputPromise->set_value(output);
  this->nextInput = nullopt;
  this->outputPromise = nullopt;
  this->engineWait.wait(this->engineLock, [this] {
    return this->nextInput.has_value();
  });
  fflush(stdout);
  return *this->nextInput;
}

string DukTapeEngineNative::compileAndExecute(string src, string filename) {
  auto env = getEnv();


  duk_push_string(this->context, src.c_str());
  duk_push_string(this->context, filename.c_str());
  if (duk_pcompile(this->context, 0) != 0) {
    return "compile failed: " + string(duk_safe_to_string(this->context, -1));;
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
