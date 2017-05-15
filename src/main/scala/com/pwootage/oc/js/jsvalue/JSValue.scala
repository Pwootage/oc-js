package com.pwootage.oc.js.jsvalue

import java.util
import java.util.function.BiConsumer

object JSValue {
  def javaToJsValue[T](obj: T): JSValue = {
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
                val jsVal = javaToJsValue(v)
                if (jsVal.isDefined) {
                  res.put(x, jsVal)
                }
              case _ => //Do nothing
            }
          }
        })
        JSMap(res)
      case x: Array[AnyRef] =>
        JSArray(x.map(javaToJsValue))
      case _ =>
        JSUndefined
    }
  }
}

/**
  * Created by pwootage on 5/14/17.
  */
trait JSValue {
  def property(name: String): JSValue = JSUndefined

  def asString: Option[String] = None

  def asDouble: Option[Double] = None

  def asBoolean: Option[Boolean] = None

  def asArray: Option[Array[JSValue]] = None

  def isDefined: Boolean = true

  /**
    * Converts this to a simple java type. Note this will return None for non-basic types!
    */
  def asSimpleJava: Option[AnyRef] = None
}

case object JSUndefined extends JSValue {
  override def isDefined = false
}

case class JSStringValue(value: String) extends JSValue {
  override def asString = Some(value)

  override def asSimpleJava = Some(value)
}

case class JSDoubleValue(value: Double) extends JSValue {
  override def asDouble = Some(value)

  override def asSimpleJava: Some[java.lang.Double] = Some(value)
}

case class JSBooleanValue(value: Boolean) extends JSValue {
  override def asBoolean = Some(value)

  override def asSimpleJava: Some[java.lang.Boolean] = Some(value)
}

case class JSArray(value: Array[JSValue]) extends JSValue {
  override def asArray = Some(value)

  override def asSimpleJava = Some(value.flatMap(_.asSimpleJava))
}

case class JSMap(value: util.HashMap[String, JSValue]) extends JSValue {
  override def property(name: String) = value.getOrDefault(name, JSUndefined)
}