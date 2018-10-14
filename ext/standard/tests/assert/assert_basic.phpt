--TEST--
assert() - basic - check  that assert runs when assert.active is set to 1.
--INI--
assert.active = 1
assert.warning = 0
assert.callback = f1
assert.bail = 0
assert.quiet_eval = 0
--FILE--
<?php
function f1()
{
	echo "f1 called\n";
}
//String assert
$sa = "0 != 0";
var_dump($r2=assert($sa));
$sa = "0 == 0";
var_dump($r2=assert($sa));

//Non string assert
var_dump($r2=assert(0));
var_dump($r2=assert(1));
--EXPECTF--
Deprecated: assert(): Calling assert() with a string argument is deprecated in %s on line %d
f1 called
bool(false)

Deprecated: assert(): Calling assert() with a string argument is deprecated in %s on line %d
bool(true)
f1 called
bool(false)
bool(true)
