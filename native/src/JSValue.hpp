#ifndef OCJS_JSVALUE_HPP
#define OCJS_JSVALUE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#ifdef JS_GENERATE_JVM_CONVERT
#include <jni.h>
#endif

struct JSValue;
struct JSStringValue;
struct JSBooleanValue;
struct JSDoubleValue;
struct JSArrayValue;
struct JSByteArrayValue;
struct JSMapValue;
struct JSNullValue;

using JSValuePtr = std::shared_ptr<JSValue>;

/** Native mirror of the Java class */
struct JSValue {
  virtual ~JSValue() = default;

  enum class Type {
    STRING,
    BOOLEAN,
    DOUBLE,
    ARRAY,
    BYTE_ARRAY,
    MAP,
    NULL_TYPE
  };

  virtual Type getType() = 0;

  JSStringValue *asString();
  JSBooleanValue *asBoolean();
  JSDoubleValue *asDouble();
  JSArrayValue *asArray();
  JSByteArrayValue *asByteArray();
  JSMapValue *asMap();
  JSNullValue *asNull();

  #ifdef JS_GENERATE_JVM_CONVERT
  virtual jobject toJVM(JNIEnv *env) = 0;
  static JSValuePtr fromJVM(JNIEnv *env, jobject obj);
  static void jvmInit(JNIEnv *env);
  #endif
};

struct JSStringValue : public JSValue {
  explicit JSStringValue(std::u16string value);
  explicit JSStringValue(const std::string& value);
  Type getType() override;
  //TODO: helper methods for to/from u8 (since we store it as u16)

  std::string getValueAsString();
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  std::u16string value;
};

struct JSBooleanValue : public JSValue {
  explicit JSBooleanValue(bool value);
  Type getType() override;
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  bool value = false;
};

struct JSDoubleValue: public JSValue {
  explicit JSDoubleValue(double value);
  Type getType() override;
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  double value = 0;
};


struct JSArrayValue : public JSValue {
  explicit JSArrayValue(std::vector<JSValuePtr> value);
  Type getType() override;
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  std::vector<JSValuePtr> value;
};

struct JSByteArrayValue : public JSValue {
  explicit JSByteArrayValue(std::vector<uint8_t> value);
  Type getType() override;
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  std::vector<uint8_t> value;
};



struct JSMapValue : public JSValue {
  explicit JSMapValue(std::unordered_map<std::u16string, JSValuePtr> value);
  Type getType() override;
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  std::unordered_map<std::u16string, JSValuePtr> value;
};

struct JSNullValue : public JSValue {
  JSNullValue();
  Type getType() override;
  #ifdef JS_GENERATE_JVM_CONVERT
  jobject toJVM(JNIEnv *env) override;
  #endif
  // no value
};


#endif //OCJS_JSVALUE_HPP
