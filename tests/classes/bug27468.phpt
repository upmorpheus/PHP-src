--TEST--
Bug #27468 (foreach in __destruct() causes segfault)
--FILE--
<?php
class foo {
    function __destruct() {
        foreach ($this->x as $x);
    }
}
new foo();
echo 'OK';
?>
--EXPECTF--
Warning: Undefined property: foo::$x in %s on line %d

Warning: Invalid argument supplied for foreach() in %sbug27468.php on line 4
OK
