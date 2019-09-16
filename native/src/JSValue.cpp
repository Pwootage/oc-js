#include "JSValue.hpp"

#include <utility>
#include <codecvt>
#include <locale>

namespace OCJS {

#if JS_GENERATE_JVM_CONVERT

bool initialized = false;
jclass JSValue_class;

jclass JSArrayValue_class;
jmethodID JSArrayValue_init;
jfieldID JSArrayValue_value;

jclass JSBooleanValue_class;
jmethodID JSBooleanValue_init;
jfieldID JSBooleanValue_value;

jclass JSByteArrayValue_class;
jmethodID JSByteArrayValue_init;
jfieldID JSByteArrayValue_value;

jclass JSDoubleValue_class;
jmethodID JSDoubleValue_init;
jfieldID JSDoubleValue_value;

jclass JSMapValue_class;
jmethodID JSMapValue_init;
jfieldID JSMapValue_value;

jclass JSNullValue_class;
jfieldID JSNullValue_INSTANCE;

jclass JSStringValue_class;
jmethodID JSStringValue_init;
jfieldID JSStringValue_value;

jclass HashMap_class;
jmethodID HashMap_init;
jmethodID HashMap_put;
jmethodID HashMap_get;
jmethodID HashMap_keySet;

jclass Set_class;
jmethodID Set_toArray;

void JSValue::jvmInit(JNIEnv *env) {
  if (initialized) return;
  initialized = true;

  jclass clazz = env->FindClass("com/pwootage/oc/js/JSValue");
  JSValue_class = (jclass) env->NewGlobalRef(clazz);
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSArray");
  JSArrayValue_class = (jclass) env->NewGlobalRef(clazz);
  JSArrayValue_init = env->GetMethodID(clazz, "<init>", "([Lcom/pwootage/oc/js/JSValue;)V");
  JSArrayValue_value = env->GetFieldID(clazz, "value", "[Lcom/pwootage/oc/js/JSValue;");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSBooleanValue");
  JSBooleanValue_class = (jclass) env->NewGlobalRef(clazz);
  JSBooleanValue_init = env->GetMethodID(clazz, "<init>", "(Z)V");
  JSBooleanValue_value = env->GetFieldID(clazz, "value", "Z");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSByteArray");
  JSByteArrayValue_class = (jclass) env->NewGlobalRef(clazz);
  JSByteArrayValue_init = env->GetMethodID(clazz, "<init>", "([B)V");
  JSByteArrayValue_value = env->GetFieldID(clazz, "value", "[B");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSDoubleValue");
  JSDoubleValue_class = (jclass) env->NewGlobalRef(clazz);
  JSDoubleValue_init = env->GetMethodID(clazz, "<init>", "(D)V");
  JSDoubleValue_value = env->GetFieldID(clazz, "value", "D");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSMap");
  JSMapValue_class = (jclass) env->NewGlobalRef(clazz);
  JSMapValue_init = env->GetMethodID(clazz, "<init>", "(Ljava/util/Map;)V");
  JSMapValue_value = env->GetFieldID(clazz, "value", "Ljava/util/Map;");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSNull");
  JSNullValue_class = (jclass) env->NewGlobalRef(clazz);
  JSNullValue_INSTANCE = env->GetStaticFieldID(clazz, "INSTANCE", "Lcom/pwootage/oc/js/JSNull;");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("com/pwootage/oc/js/JSStringValue");
  JSStringValue_class = (jclass) env->NewGlobalRef(clazz);
  JSStringValue_init = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  JSStringValue_value = env->GetFieldID(clazz, "value", "Ljava/lang/String;");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("java/util/HashMap");
  HashMap_class = (jclass) env->NewGlobalRef(clazz);
  HashMap_init = env->GetMethodID(clazz, "<init>", "()V");
  HashMap_put = env->GetMethodID(clazz, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  HashMap_get = env->GetMethodID(clazz, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
  HashMap_keySet = env->GetMethodID(clazz, "keySet", "()Ljava/util/Set;");
  env->DeleteLocalRef(clazz);

  clazz = env->FindClass("java/util/Set");
  Set_class = (jclass) env->NewGlobalRef(clazz);
  Set_toArray = env->GetMethodID(Set_class, "toArray", "()[Ljava/lang/Object;");
  env->DeleteLocalRef(clazz);
}

JSValuePtr JSValue::fromJVM(JNIEnv *env, jobject obj) {
  //TODO: add infinite recursion protection (in case they created a cyclic object somehow)

  if (env->IsInstanceOf(obj, JSStringValue_class)) {
    jstring jstr = (jstring) env->GetObjectField(obj, JSStringValue_value);
    const jchar *jstrChars = env->GetStringChars(jstr, nullptr);
    std::u16string str(reinterpret_cast<const char16_t *>(jstrChars), env->GetStringLength(jstr));
    env->ReleaseStringChars(jstr, jstrChars);
    return JSValuePtr(new JSStringValue(str));
  }
  if (env->IsInstanceOf(obj, JSBooleanValue_class)) {
    jboolean b = env->GetBooleanField(obj, JSBooleanValue_value);
    return JSValuePtr(new JSBooleanValue(b));
  }
  if (env->IsInstanceOf(obj, JSDoubleValue_class)) {
    jdouble d = env->GetDoubleField(obj, JSDoubleValue_value);
    return JSValuePtr(new JSDoubleValue(d));
  }
  if (env->IsInstanceOf(obj, JSArrayValue_class)) {
    jobjectArray arr = (jobjectArray) env->GetObjectField(obj, JSArrayValue_value);
    std::vector<JSValuePtr> res;
    res.resize(env->GetArrayLength(arr));
    for (size_t i = 0; i < res.size(); i++) {
      jobject ele = env->GetObjectArrayElement(arr, i);
      res[i] = JSValue::fromJVM(env, ele);
      env->DeleteLocalRef(ele);
    }
    return JSValuePtr(new JSArrayValue(res));
  }
  if (env->IsInstanceOf(obj, JSByteArrayValue_class)) {
    jbyteArray arr = (jbyteArray) env->GetObjectField(obj, JSByteArrayValue_value);
    std::vector<uint8_t> bytes;
    bytes.resize(env->GetArrayLength(arr));
    env->GetByteArrayRegion(arr, 0, bytes.size(), reinterpret_cast<jbyte *>(bytes.data()));
    return JSValuePtr(new JSByteArrayValue(bytes));
  }
  if (env->IsInstanceOf(obj, JSMapValue_class)) {
    jobject map = env->GetObjectField(obj, JSMapValue_value);
    jobject keySet = env->CallObjectMethod(map, HashMap_keySet);
    jobjectArray arr = (jobjectArray) env->CallObjectMethod(keySet, Set_toArray);

    std::unordered_map<std::u16string, JSValuePtr> res;
    size_t len = env->GetArrayLength(arr);
    for (size_t i = 0; i < len; i++) {
      jstring key = (jstring) env->GetObjectArrayElement(arr, i);
      const jchar *jstrChars = env->GetStringChars(key, nullptr);
      std::u16string keyStr(reinterpret_cast<const char16_t *>(jstrChars), env->GetStringLength(key));
      env->ReleaseStringChars(key, jstrChars);

      jobject value = env->CallObjectMethod(map, HashMap_get, key);

      res[keyStr] = JSValue::fromJVM(env, value);

      env->DeleteLocalRef(key);
      env->DeleteLocalRef(value);
    }

    return JSValuePtr(new JSMapValue(res));
  }
  if (env->IsInstanceOf(obj, JSNullValue_class)) {
    return JSValuePtr(new JSNullValue());
  }

  return std::make_unique<JSNullValue>();
}

jobject JSStringValue::toJVM(JNIEnv *env) {
  // jchar is 16bit so this is compatible
  jstring str = env->NewString(reinterpret_cast<const jchar *>(value.c_str()), value.length());
  return env->NewObject(JSStringValue_class, JSStringValue_init, str);
}

jobject JSBooleanValue::toJVM(JNIEnv *env) {
  return env->NewObject(JSBooleanValue_class, JSBooleanValue_init, value);
}

jobject JSDoubleValue::toJVM(JNIEnv *env) {
  return env->NewObject(JSDoubleValue_class, JSDoubleValue_init, value);
}

jobject JSArrayValue::toJVM(JNIEnv *env) {
  jobjectArray array = env->NewObjectArray(value.size(), JSValue_class, nullptr);
  for (size_t i = 0; i < value.size(); i++) {
    jobject child = value[i]->toJVM(env);
    env->SetObjectArrayElement(array, i, child);
    env->DeleteLocalRef(child);
  }

  return env->NewObject(JSArrayValue_class, JSArrayValue_init, array);
}

jobject JSByteArrayValue::toJVM(JNIEnv *env) {
  jbyteArray array = env->NewByteArray(value.size());
  env->SetByteArrayRegion(array, 0, value.size(), reinterpret_cast<const jbyte *>(value.data()));
  return env->NewObject(JSByteArrayValue_class, JSByteArrayValue_init, array);
}

jobject JSMapValue::toJVM(JNIEnv *env) {
  jobject map = env->NewObject(HashMap_class, HashMap_init);
  for (const auto &v : value) {
    jstring str = env->NewString(reinterpret_cast<const jchar *>(v.first.c_str()), v.first.length());
    jobject obj = v.second->toJVM(env);
    env->CallObjectMethod(map, HashMap_put, str, obj);
    env->DeleteLocalRef(str);
    env->DeleteLocalRef(obj);
  }

  return env->NewObject(JSMapValue_class, JSMapValue_init, map);
}

jobject JSNullValue::toJVM(JNIEnv *env) {
  return env->GetStaticObjectField(JSNullValue_class, JSNullValue_INSTANCE);
}

#endif

JSStringValue *JSValue::asString() {
  if (getType() == Type::STRING) {
    return (JSStringValue *) this;
  } else {
    return nullptr;
  }
}

JSBooleanValue *JSValue::asBoolean() {
  if (getType() == Type::BOOLEAN) {
    return (JSBooleanValue *) this;
  } else {
    return nullptr;
  }
}

JSDoubleValue *JSValue::asDouble() {
  if (getType() == Type::DOUBLE) {
    return (JSDoubleValue *) this;
  } else {
    return nullptr;
  }
}

JSArrayValue *JSValue::asArray() {
  if (getType() == Type::ARRAY) {
    return (JSArrayValue *) this;
  } else {
    return nullptr;
  }
}

JSByteArrayValue *JSValue::asByteArray() {
  if (getType() == Type::BYTE_ARRAY) {
    return (JSByteArrayValue *) this;
  } else {
    return nullptr;
  }
}

JSMapValue *JSValue::asMap() {
  if (getType() == Type::MAP) {
    return (JSMapValue *) this;
  } else {
    return nullptr;
  }
}

JSNullValue *JSValue::asNull() {
  if (getType() == Type::NULL_TYPE) {
    return (JSNullValue *) this;
  } else {
    return nullptr;
  }
}

JSStringValue::JSStringValue(std::u16string value) : value(std::move(value)) {}

JSStringValue::JSStringValue(const std::string &value) {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  this->value = convert.from_bytes(value);
}

JSValue::Type JSStringValue::getType() {
  return JSValue::Type::STRING;
}

std::string JSStringValue::getValueAsString() {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  return convert.to_bytes(value);
}

JSBooleanValue::JSBooleanValue(bool value) : value(value) {}

JSValue::Type JSBooleanValue::getType() {
  return JSValue::Type::BOOLEAN;
}

JSDoubleValue::JSDoubleValue(double value) : value(value) {}

JSValue::Type JSDoubleValue::getType() {
  return JSValue::Type::DOUBLE;
}

JSArrayValue::JSArrayValue(std::vector<JSValuePtr> value) : value(std::move(value)) {}

JSValue::Type JSArrayValue::getType() {
  return JSValue::Type::ARRAY;
}

// TODO: is this copying the data? it's been too long
JSByteArrayValue::JSByteArrayValue(std::vector<uint8_t> value) : value(std::move(value)) {}

JSValue::Type JSByteArrayValue::getType() {
  return JSValue::Type::BYTE_ARRAY;
}

JSMapValue::JSMapValue(std::unordered_map<std::u16string, JSValuePtr> value) : value(std::move(value)) {}

JSValue::Type JSMapValue::getType() {
  return JSValue::Type::MAP;
}

JSNullValue::JSNullValue() = default;

JSValue::Type JSNullValue::getType() {
  return JSValue::Type::NULL_TYPE;
}

}
