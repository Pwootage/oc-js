package com.pwootage.oc.js

import com.google.gson.*

/**
 * Created by pwootage on 5/14/17.
 */
sealed class JSValue {
  open fun property(name: String): JSValue = JSNull

  open fun arrayVal(index: Int): JSValue = JSNull

  open val asString: String? get() = null

  open val asDouble: Double? get() = null

  open val asBoolean: Boolean? get() = null

  open val asArray: Array<JSValue>? get() = null

  /**
   * Converts this to a simple java type. Note this will return None for non-basic types!
   */
  abstract val asSimpleJava: Any?

  abstract fun toJSON(): String

  companion object {
    val gson = GsonBuilder().create()

    fun fromJSON(json: String): JSValue {
      val jsonParser = JsonParser()
      val ele = jsonParser.parse(json)
      return fromJSON(ele)
    }

    fun fromJSON(ele: JsonElement): JSValue {
      return when (ele) {
        is JsonPrimitive ->
          when {
            ele.isBoolean -> JSBooleanValue(ele.getAsBoolean())
            ele.isNumber -> JSDoubleValue(ele.getAsDouble())
            else -> JSStringValue(ele.getAsString())
          }
        is JsonArray -> {
          val res = Array(ele.size()) {
            fromJSON(ele[it])
          }
          JSArray(res)
        }
        is JsonObject -> {
          val res = HashMap<String, JSValue>()
          for (entry in ele.entrySet()) {
            res[entry.key] = fromJSON(entry.value)
          }
          JSMap(res)
        }
        is JsonNull -> JSNull
        else -> {
          println("Unknown json? $ele")
          JSNull
        }
      }
    }

    fun <T : Any> fromJava(obj: T?): JSValue {
      return when (obj) {
        is Float -> JSDoubleValue(obj.toDouble())
        is Double -> JSDoubleValue(obj.toDouble())
        is Int -> JSDoubleValue(obj.toDouble())
        is Long -> JSDoubleValue(obj.toDouble())
        is Boolean -> JSBooleanValue(obj)
        is String -> JSStringValue(obj)
        is HashMap<*, *> -> {
          val res = HashMap<String, JSValue>()
          obj.forEach {
            val k = it.key as? String
            if (k != null)
              res[k] = fromJava(it.value)
          }
          JSMap(res)
        }
        is ByteArray -> JSByteArray(obj)
        is Array<*> -> JSArray(obj.map(::fromJava).toTypedArray())
        is ArrayList<*> -> JSArray(obj.map(::fromJava).toTypedArray())
        null -> JSNull
        else -> {
          println("Unknown fromJava: ${obj::class.java} $obj")
          fromJSON(gson.toJson(obj))
        }
      }
    }
  }
}

data class JSStringValue(val value: String) : JSValue() {
  override val asString = value

  override val asSimpleJava: String = value

  override fun toJSON(): String = gson.toJson(value)
}

data class JSDoubleValue(val value: Double) : JSValue() {

  override val asDouble = value

  override val asSimpleJava = value

  override fun toJSON() = String.format("%f", value);
}

data class JSBooleanValue(val value: Boolean) : JSValue() {
  override val asBoolean = value

  override val asSimpleJava = value

  override fun toJSON() = if (value) "true" else "false"
}

data class JSArray(val value: Array<JSValue>) : JSValue() {
  override fun arrayVal(index: Int): JSValue {
    return if (index >= 0 && index < value.size) {
      value[index]
    } else {
      JSNull
    }
  }

  override val asArray = value

  override val asSimpleJava = value.map { it.asSimpleJava }.toTypedArray()

  override fun toJSON() = "[${value.joinToString(",") { it.toJSON() }}]"
}

data class JSMap(val value: Map<String, JSValue>) : JSValue() {
  override fun property(name: String) = value[name] ?: JSNull

  override val asSimpleJava: Map<String, Any?>
    get() {
      val res = HashMap<String, Any?>()
      value.forEach {
        res[it.key] = it.value.asSimpleJava
      }
      return res
    }

  override fun toJSON() = "{" +
    value.map { gson.toJson(it.key) + ":" + it.value.toJSON() }.joinToString(",") +
    "}"
}

data class JSByteArray(val value: ByteArray) : JSValue() {
  override val asSimpleJava = value

  override fun toJSON(): String {
    TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
  }
}

object JSNull : JSValue() {
  override fun toJSON() = "null"

  override val asSimpleJava: Nothing? = null
}