--TEST--
Const string dereference
--FILE--
<?php
error_reporting(E_ALL);

var_dump("foobar"[3]);
var_dump("foobar"[2][0]);
var_dump("foobar"["foo"]["bar"]);
--EXPECTF--
string(1) "b"
string(1) "o"

Warning: Illegal string offset "foo" in %s on line %d

Warning: Illegal string offset "bar" in %s on line %d
string(1) "f"
