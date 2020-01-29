--TEST--
call_user_func() should not use FUNC_ARG fetches
--FILE--
<?php

function foo(&$ref) { $ref = 24; }

$a = [];
call_user_func('foo', $a[0][0]);
var_dump($a);

?>
--EXPECTF--
Notice: Undefined offset: 0 in %s on line %d

Warning: Trying to access array offset on value of type null in %s on line %d

Warning: foo() expects argument #1 ($ref) to be passed by reference, value given in %s on line %d
array(0) {
}
