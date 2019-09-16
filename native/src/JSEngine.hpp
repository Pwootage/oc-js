#ifndef OCJS_JSENGINE_HPP
#define OCJS_JSENGINE_HPP

#include <memory>
#include <functional>
#include <future>
#include <string>
#include <mutex>
#include <optional>
#include "./JSValue.hpp"

class JSEngine {
public:
  virtual ~JSEngine() = default;

  virtual std::future<JSValuePtr> next(JSValuePtr next) = 0;
  [[nodiscard]] virtual size_t getMaxMemory() const = 0;
  virtual void setMaxMemory(size_t maxMemory) = 0;
  [[nodiscard]] virtual size_t getAllocatedMemory() const = 0;
};

#endif //OCJS_JSENGINE_HPP
