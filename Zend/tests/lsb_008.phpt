--TEST--
ZE2 Late Static Binding class name "static"
--FILE--
<?php
class static {
}
--EXPECTF--
Parse error: parse error, expecting `T_STRING' in %s on line %d

