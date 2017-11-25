package com.pwootage.oc.js.jsvalue

import java.nio.charset.StandardCharsets
import java.text.NumberFormat
import java.util
import java.util.function.BiConsumer

import com.google.gson._

import scala.collection.JavaConversions._

/**
  * Basically a small json library wrapping gson because I'm too lazy to figure out an external dep
  */
object JSValue {
  val gson = new GsonBuilder().create()

  def fromJSON(json: String): JSValue = {
    val jsonParser = new JsonParser()
    val ele = jsonParser.parse(json)
    JSValue.fromJSON(ele)
  }

  def fromJSON(ele: JsonElement): JSValue = {
    ele match {
      case primitive: JsonPrimitive if ele.isJsonPrimitive =>
        if (primitive.isBoolean) {
          JSBooleanValue(primitive.getAsBoolean)
        } else if (primitive.isNumber) {
          JSDoubleValue(primitive.getAsDouble)
        } else {
          JSStringValue(primitive.getAsString)
        }
      case array: JsonArray if ele.isJsonArray =>
        val res = new Array[JSValue](array.size())
        for (i <- res.indices) {
          res(i) = fromJSON(array.get(i))
        }
        JSArray(res)
      case obj: JsonObject if ele.isJsonObject =>
        val res = new util.HashMap[String, JSValue]()
        for (entry <- obj.entrySet().iterator()) {
          res.put(entry.getKey, fromJSON(entry.getValue))
        }
        JSMap(res)
      case nul: JsonNull if ele.isJsonNull =>
        JSNull
      case x =>
        println("Unknown json?", x)
        JSNull
    }
  }

  def fromJava[T](obj: T): JSValue = {
    obj match {
      case x: java.lang.Float => JSDoubleValue(x.doubleValue())
      case x: java.lang.Double => JSDoubleValue(x.doubleValue())
      case x: java.lang.Integer => JSDoubleValue(x.doubleValue())
      case x: java.lang.Long => JSDoubleValue(x.doubleValue())
      case x: java.lang.Boolean => JSBooleanValue(x)
      case x: String => JSStringValue(x)
      case x: util.HashMap[kt, vt] =>
        val res = new util.HashMap[String, JSValue]()
        x.forEach(new BiConsumer[kt, vt] {
          override def accept(k: kt, v: vt): Unit = {
            k match {
              case x: String =>
                val jsVal = fromJava(v)
                res.put(x, jsVal)
              case _ => //Do nothing
            }
          }
        })
        JSMap(res)
      case x: Array[Byte] =>
        //TODO: arraybuffers?
        JSStringValue(new String(x, StandardCharsets.UTF_8))
      case x: Array[AnyRef] =>
        JSArray(x.map(fromJava))
      case x: util.ArrayList[_] =>
        JSArray(x.map(fromJava).toArray)
      case null =>
        JSNull
      case x =>
        JSValue.fromJSON(JSValue.gson.toJson(x))
      //        JSStringValue(x.toString)
    }
  }
}

/**
  * Created by pwootage on 5/14/17.
  */
trait JSValue {
  def property(name: String): JSValue = JSNull

  def arrayVal(index: Int): JSValue = JSNull

  def asString: Option[String] = None

  def asDouble: Option[Double] = None

  def asBoolean: Option[Boolean] = None

  def asArray: Option[Array[JSValue]] = None

  /**
    * Converts this to a simple java type. Note this will return None for non-basic types!
    */
  def asSimpleJava: AnyRef

  def toJSON: String
}

case class JSStringValue(value: String) extends JSValue {
  override def asString = Some(value)

  override def asSimpleJava: String = value

  override def toJSON = JSValue.gson.toJson(value)
}

case class JSDoubleValue(value: Double) extends JSValue {

  override def asDouble = Some(value)

  def asLong = {
    val longV = value.toLong
    if (longV == value) {
      Some(longV)
    } else {
      None
    }
  }

  def asInt = {
    val intV = value.toInt
    if (intV == value) {
      Some(intV)
    } else {
      None
    }
  }

  override def asSimpleJava = asInt.map(new java.lang.Integer(_))
    .orElse(asLong.map(new java.lang.Long(_)))
    .getOrElse(new java.lang.Double(value))

  override def toJSON = value.formatted("%f")
}

case class JSBooleanValue(value: Boolean) extends JSValue {
  override def asBoolean = Some(value)

  override def asSimpleJava: java.lang.Boolean = value

  override def toJSON = if (value) "true" else "false"
}

case class JSArray(value: Array[JSValue]) extends JSValue {
  override def arrayVal(index: Int) = {
    if (index >= 0 && index < value.length) {
      value(index)
    } else {
      JSNull
    }
  }

  override def asArray = Some(value)

  override def asSimpleJava = value.map(_.asSimpleJava)

  override def toJSON = s"[${value.map(_.toJSON).mkString(",")}]"
}

case class JSMap(value: util.HashMap[String, JSValue]) extends JSValue {
  override def property(name: String) = value.getOrDefault(name, JSNull)

  override def asSimpleJava: util.HashMap[String, AnyRef] = {
    val res = new util.HashMap[String, AnyRef]()
    value.entrySet().foreach(v => res.put(v.getKey, v.getValue.asSimpleJava))
    res
  }

  override def toJSON = "{" +
    value.entrySet()
      .map(entry => JSValue.gson.toJson(entry.getKey) + ":" + entry.getValue.toJSON)
      .mkString(",") +
    "}"
}

object JSMap {
  def apply(pairs: (String, JSValue)*): JSMap = {
    val res = new util.HashMap[String, JSValue]()
    for ((key, value)<- pairs) {
      res.put(key, value)
    }
    JSMap(res)
  }
}

case object JSNull extends JSValue {
  override def toJSON = "null"

  override def asSimpleJava = null
}