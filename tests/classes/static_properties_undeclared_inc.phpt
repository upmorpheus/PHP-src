--TEST--
Incrementing a non-existent static property
--FILE--
<?php
Class C {}
C::$p++;
?>
--EXPECTF--
Fatal error: Uncaught exception 'EngineException' with message 'Access to undeclared static property: C::$p' in %s:3
Stack trace:
#0 {main}
  thrown in %s on line 3
