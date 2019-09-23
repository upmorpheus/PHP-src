--TEST--
Typed properties in internal classes
--SKIPIF--
<?php if (!extension_loaded('zend-test')) die('skip requires zend-test'); ?>
--FILE--
<?php

// Internal typed properties

$obj = new _ZendTestClass;
var_dump($obj->intProp);
try {
    $obj->intProp = "foobar";
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
$obj->intProp = 456;

try {
    $obj->classProp = $obj;
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
$obj->classProp = new stdClass;
var_dump($obj);

// Inherit from internal class

class Test extends _ZendTestClass {
}

$obj = new Test;
var_dump($obj->intProp);
try {
    $obj->intProp = "foobar";
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
$obj->intProp = 456;

try {
    $obj->classProp = $obj;
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
$obj->classProp = new stdClass;
var_dump($obj);

// Static internal typed properties

var_dump(_ZendTestClass::$staticIntProp);
try {
    _ZendTestClass::$staticIntProp = "foobar";
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
_ZendTestClass::$staticIntProp = 456;
var_dump(_ZendTestClass::$staticIntProp);

?>
--EXPECT--
int(123)
Cannot assign string to property _ZendTestClass::$intProp of type int
Cannot assign _ZendTestClass to property _ZendTestClass::$classProp of type ?stdClass
object(_ZendTestClass)#1 (2) {
  ["intProp"]=>
  int(456)
  ["classProp"]=>
  object(stdClass)#2 (0) {
  }
}
int(123)
Cannot assign string to property _ZendTestClass::$intProp of type int
Cannot assign Test to property _ZendTestClass::$classProp of type ?stdClass
object(Test)#4 (2) {
  ["intProp"]=>
  int(456)
  ["classProp"]=>
  object(stdClass)#1 (0) {
  }
}
int(123)
Cannot assign string to property _ZendTestClass::$staticIntProp of type int
int(456)
