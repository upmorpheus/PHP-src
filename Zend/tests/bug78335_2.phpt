--TEST--
Bug #78335: Static properties containing cycles report as leak (internal class variant)
--FILE--
<?php

$foo = [&$foo];
_ZendTestClass::$_StaticProp = $foo;

?>
===DONE===
--EXPECT--
===DONE===
