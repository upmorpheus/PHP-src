--TEST--
preg_replace_callback_array() basic functions
--FILE--
<?php

function f() {
	throw new Exception();
}

try {
var_dump(preg_replace_callback_array(array('/\w/' => 'f'), 'z'));
} catch(Exception $e) {}

function g($x) {
	return "'$x[0]'";
}

var_dump(preg_replace_callback_array(array('@\b\w{1,2}\b@' => 'g'), array('a b3 bcd', 'v' => 'aksfjk', 12 => 'aa bb')));

var_dump(preg_replace_callback_array(array('~\A.~' => 'g'), array(array('xyz'))));

var_dump(preg_replace_callback_array(array('~\A.~' => create_function('$m', 'return strtolower($m[0]);')), 'ABC'));
?>
--EXPECTF--
array(3) {
  [0]=>
  string(12) "'a' 'b3' bcd"
  ["v"]=>
  string(6) "aksfjk"
  [12]=>
  string(9) "'aa' 'bb'"
}

Notice: Array to string conversion in %spreg_replace_callback_array.php on line %d
array(1) {
  [0]=>
  string(7) "'A'rray"
}
string(3) "aBC"
