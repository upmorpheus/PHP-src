--TEST--
Bug #75420.12 (Indirect modification of magic method argument)
--FILE--
<?php
class Test implements ArrayAccess {
    public function offsetExists($x) { $GLOBALS["name"] = 24; return true; }
    public function offsetGet($x) { var_dump($x); return 42; }
    public function offsetSet($x, $y) { }
    public function offsetUnset($x) { }
}

$obj = new Test;
$name = "foo";
$name = str_repeat($name, 2);
var_dump(empty($obj[$name]));
var_dump($name);
?>
--EXPECT--
string(6) "foofoo"
bool(false)
int(24)
