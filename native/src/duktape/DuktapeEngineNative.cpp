//
// Created by pwootage on 11/14/17.
//
#include "DuktapeEngineNative.h"

#include <cstdio>
#include <thread>
#include <functional>
#include <locale>
#include <codecvt>

using namespace std;

constexpr int MAX_OBJ_KEYS = 1000;

duk_bool_t duk_exec_timeout(void *udata) {
  return static_cast<DukTapeEngineNative *>(udata)->shouldKill;
}

DukTapeEngineNative::DukTapeEngineNative() {
  this->mainThread = thread([this] { this->mainThreadFn(); });
}

DukTapeEngineNative::~DukTapeEngineNative() {
  // TODO: maybe a safer/cleaner thread kill
  debug_print("JS main thread kill");
  shouldKill = true;
  if (!isDead) {
    debug_print("JS main thread is not dead");
    //this->next(R"({"state": "error", "value": "kill"})");
    pthread_cancel(this->mainThread.native_handle());
  }
  debug_print("JS waiting for main thread");
  this->mainThread.join();

  duk_destroy_heap(this->context);
  this->context = nullptr;
  debug_print("JS main thread kill complete");
}

future<OCJS::JSValuePtr> DukTapeEngineNative::next(OCJS::JSValuePtr next) {
  future<OCJS::JSValuePtr> res;
  {
    lock_guard<mutex> lock(this->executionMutex);

    if (this->deadResult) {
      promise<OCJS::JSValuePtr> promise;
      promise.set_value(*this->deadResult);
      return promise.get_future();
    }

    this->nextInput = make_optional(next);
    this->outputPromise = make_optional(promise<OCJS::JSValuePtr>());
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
  std::unordered_map<std::u16string, OCJS::JSValuePtr> map;
  map[u"type"] = OCJS::JSValuePtr(new OCJS::JSStringValue("__bios__"));
  OCJS::JSValuePtr src = this->yield(OCJS::JSValuePtr(new OCJS::JSMapValue(map)));
  OCJS::JSValuePtr res;
  if (src->getType() != OCJS::JSValue::Type::STRING) {
    // todo: throw java exception
    res = OCJS::JSValuePtr(new OCJS::JSStringValue("must provide a string for the first yield"));
  } else {
    res = this->compileAndExecute(src->asString()->getValueAsString(), "__bios__");
  }

  this->deadResult = make_optional(res);
  if (this->outputPromise.has_value()) {
    this->outputPromise->set_value(res);
  }
  // We're now dead and can't execute anymore. :'(
  this->isDead = true;

  debug_print("JS main thread end");

  this->engineLock.unlock();
}

OCJS::JSValuePtr DukTapeEngineNative::yield(const OCJS::JSValuePtr &output) {
  this->outputPromise->set_value(output);
  this->nextInput = nullopt;
  this->outputPromise = nullopt;
  this->engineWait.wait(this->engineLock, [this] {
    return this->nextInput.has_value();
  });
  fflush(stdout);
  return *this->nextInput;
}

OCJS::JSValuePtr DukTapeEngineNative::compileAndExecute(const string &src, const string &filename) {
  duk_push_string(this->context, src.c_str());
  duk_push_string(this->context, filename.c_str());
  if (duk_pcompile(this->context, 0) != 0) {
    return OCJS::JSValuePtr(new OCJS::JSStringValue(
      "compile failed: " + string(duk_safe_to_string(this->context, -1))
    ));
  }
  if (duk_pcall(this->context, 0) != 0) {
    return OCJS::JSValuePtr(new OCJS::JSStringValue(
      "pcall failed: " + string(duk_safe_to_string(this->context, -1))
    ));
  }
  return convertObjectToJSValue(-1);
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


  duk_gc(ctx, 0);

  // pull out args
  OCJS::JSValuePtr args = native->convertObjectToJSValue(0);

  // yield
  OCJS::JSValuePtr res = native->yield(args);

  // Return
  native->pushJSValue(res);
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
  OCJS::JSValuePtr deadResult = OCJS::JSValuePtr(new OCJS::JSStringValue(R"({"state": "error", "value": "kill: )" + smsg + R"("})"));
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

void DukTapeEngineNative::pushJSValue(const OCJS::JSValuePtr &ptr) {
  duk_context *ctx = this->context;

  switch (ptr->getType()) {
    case OCJS::JSValue::Type::STRING: {
      auto str = ptr->asString()->getValueAsString();
      duk_push_lstring(ctx, str.c_str(), str.length());
      break;
    }
    case OCJS::JSValue::Type::BOOLEAN: {
      duk_push_boolean(ctx, ptr->asBoolean()->value);
      break;
    }
    case OCJS::JSValue::Type::DOUBLE: {
      duk_push_number(ctx, ptr->asDouble()->value);
      break;
    }
    case OCJS::JSValue::Type::ARRAY: {
      auto &arr = ptr->asArray()->value;
      duk_push_array(ctx);
      for (size_t i = 0; i < arr.size(); i++) {
        pushJSValue(arr[i]);
        duk_put_prop_index(ctx, -2, i);
      }
      break;
    }
    case OCJS::JSValue::Type::BYTE_ARRAY: {
      auto &buf = ptr->asByteArray()->value;
      duk_push_fixed_buffer(ctx, buf.size());
      void *dukBuff = duk_get_buffer_data(ctx, -1, nullptr);
      memcpy(dukBuff, buf.data(), buf.size());
      duk_push_buffer_object(ctx, -1, 0, buf.size(), DUK_BUFOBJ_UINT8ARRAY);
      duk_remove(ctx, -2);
      break;
    }
    case OCJS::JSValue::Type::MAP: {
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
      auto &map = ptr->asMap()->value;
      duk_push_object(ctx);
      for (auto &ele : map) {
        std::string key = convert.to_bytes(ele.first);
        pushJSValue(ele.second);
        duk_put_prop_lstring(ctx, -2, key.c_str(), key.length());
      }
      break;
    }
    case OCJS::JSValue::Type::NULL_TYPE: {
      duk_push_null(ctx);
      break;
    }
    default: {
      debug_print("Pushing nothing?!?");
    }
  }
}

OCJS::JSValuePtr DukTapeEngineNative::convertObjectToJSValue(duk_idx_t idx) {
  duk_context *ctx = this->context;
  duk_int_t type = duk_get_type(ctx, idx);

  switch (type) {
    case DUK_TYPE_BOOLEAN: {
      duk_bool_t v = duk_require_boolean(ctx, idx);
      return OCJS::JSValuePtr(new OCJS::JSBooleanValue(v));
    }
    case DUK_TYPE_NUMBER: {
      duk_double_t d = duk_require_number(ctx, idx);
      return OCJS::JSValuePtr(new OCJS::JSDoubleValue(d));
    }
    case DUK_TYPE_STRING: {
      duk_size_t len = 0;
      const char *chars = duk_require_lstring(ctx, idx, &len);
      std::string str(chars, len);
      return OCJS::JSValuePtr(new OCJS::JSStringValue(str));
    }
    case DUK_TYPE_BUFFER:
    case DUK_TYPE_OBJECT : {
      if (duk_is_buffer_data(ctx, idx)) {
        duk_size_t size = 0;
        void *buffData = duk_require_buffer_data(ctx, idx, &size);
        std::vector<uint8_t> resBuff;
        resBuff.resize(size);
        memcpy(resBuff.data(), buffData, size);
        return OCJS::JSValuePtr(new OCJS::JSByteArrayValue(resBuff));
      } else if (duk_is_array(ctx, idx)) {
        duk_size_t len = duk_get_length(ctx, idx);
        if (len > MAX_OBJ_KEYS) len = MAX_OBJ_KEYS;
        std::vector<OCJS::JSValuePtr> resVec;
        resVec.resize(len);
        for (size_t i = 0; i < len; i++) {
          duk_get_prop_index(ctx, idx, i);
          resVec[i] = convertObjectToJSValue(-1);
          duk_pop(ctx);
        }
        return OCJS::JSValuePtr(new OCJS::JSArrayValue(resVec));
      } else {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        std::unordered_map<std::u16string, OCJS::JSValuePtr> res;
        // is an object
        size_t safety = 0;
        duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (safety < MAX_OBJ_KEYS && duk_next(ctx, -1, 1)) {
          safety++;
          size_t keyLen = 0;
          const char *keyChars = duk_safe_to_lstring(ctx, -2, &keyLen);
          std::string keyStr(keyChars, keyLen);
          std::u16string key = convert.from_bytes(keyStr);
          res[key] = convertObjectToJSValue(-1);
          duk_pop_2(ctx);
        }
        duk_pop(ctx);
        return OCJS::JSValuePtr(new OCJS::JSMapValue(res));
      }
    }
    case DUK_TYPE_POINTER:
    case DUK_TYPE_LIGHTFUNC:
    case DUK_TYPE_UNDEFINED:
    case DUK_TYPE_NULL:
    case DUK_TYPE_NONE:
    default:
      return OCJS::JSValuePtr(new OCJS::JSNullValue());
  }
}
