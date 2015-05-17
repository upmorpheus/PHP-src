--TEST--
Crash when calling a method of a class that inherits PDOStatement
--SKIPIF--
<?php
if (!extension_loaded('pdo')) die('skip');
?>
--FILE--
<?php

class MyStatement extends PDOStatement
{
}

$obj = new MyStatement;
var_dump($obj->foo());

?>
--EXPECTF--
Fatal error: Uncaught EngineException: Call to undefined method MyStatement::foo() in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
