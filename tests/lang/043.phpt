--TEST--
Dynamic call for static methods
--FILE--
<?php
class A {
    static function foo() { return 'foo'; }
}

$classname       =  'A';
$wrongClassname  =  'B';

echo $classname::foo()."\n";
echo $wrongClassname::foo()."\n";
?>
===DONE===
--EXPECTF--
foo

Fatal error: Uncaught exception 'EngineException' with message 'Class 'B' not found' in %s043.php:%d
Stack trace:
#0 {main}
  thrown in %s043.php on line %d
