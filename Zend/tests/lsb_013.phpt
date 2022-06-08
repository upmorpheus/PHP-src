--TEST--
ZE2 Late Static Binding is_callable() and static::method()
--FILE--
<?php

class Test1 {
    static function test() {
        var_dump(is_callable("static::ok"));
        var_dump(is_callable(array("static","ok")));
    }
}

class Test2 extends Test1 {
    static function ok() {
    }
}
Test1::test();
Test2::test();
?>
--EXPECTF--
Deprecated: Use of "static" in callables is deprecated in %s on line %d
bool(false)

Deprecated: Use of "static" in callables is deprecated in %s on line %d
bool(false)

Deprecated: Use of "static" in callables is deprecated in %s on line %d
bool(true)

Deprecated: Use of "static" in callables is deprecated in %s on line %d
bool(true)
