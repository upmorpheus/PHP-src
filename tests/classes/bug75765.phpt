--TEST--
Ensure that extending of undefined class throws the exception
--FILE--
<?php

try {
	class A extends B {}
} catch (Error $e) {
	var_dump(class_exists('A'));
	var_dump(class_exists('B'));
	throw $e;
}

?>
--EXPECTF--
bool(false)
bool(false)

Fatal error: Uncaught Error: Class 'B' not found in %sbug75765.php:%d
Stack trace:
#0 {main}
  thrown in %sbug75765.php on line %d
