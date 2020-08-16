--TEST--
Test is_countable() function
--CREDITS--
Gabriel Caruso (carusogabriel34@gmail.com)
--FILE--
<?php
var_dump(is_countable([1, 2, 3]));
var_dump(is_countable((array) 1));
var_dump(is_countable((object) ['foo', 'bar', 'baz']));

$foo = ['', []];

if (is_countable($foo)) {
    var_dump(count($foo));
}

$bar = null;
if (!is_countable($bar)) {
    count($bar);
}
?>
--EXPECTF--
bool(true)
bool(true)
bool(false)
int(2)

Warning: count(): Argument #1 ($var) must be of type Countable|array, null given in %s on line %d
