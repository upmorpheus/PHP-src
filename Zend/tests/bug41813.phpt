--TEST--
Bug #41813 (segmentation fault when using string offset as an object)
--FILE--
<?php

$foo = "50";
$foo[0]->bar = "xyz";

echo "Done\n";
?>
--EXPECTF--	
Fatal error: Uncaught exception 'Error' with message 'Cannot use string offset as an array' in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
