--TEST--
readline_add_history(): Basic test
--FILE--
<?php

var_dump(readline_add_history('foo'));
var_dump(readline_list_history());
var_dump(readline_add_history(NULL));
var_dump(readline_list_history());
var_dump(readline_clear_history());
var_dump(readline_add_history());

?>
--EXPECTF--
bool(true)
array(1) {
  [0]=>
  string(3) "foo"
}
bool(true)
array(2) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(0) ""
}
bool(true)

Warning: Wrong parameter count for readline_add_history() in %s on line %d
NULL
