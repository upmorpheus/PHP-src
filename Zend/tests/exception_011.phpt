--TEST--
Test exception doesn't cause RSHUTDOWN bypass, variation 0
--INI--
assert.bail=1
assert.exception=1
--FILE--
<?php 

define ("XXXXX", 1);
assert(false);

?>
--EXPECTF--
Fatal error: Uncaught exception 'AssertionException' with message 'assert(false)' in %sexception_011.php:%d
Stack trace:
#0 %sexception_011.php(%d): assert(false, 'assert(false)')
#1 {main}
  thrown in %sexception_011.php on line %d
