--TEST--
Bug #26182 (Object properties created redundantly)
--INI--
error_reporting=2039
--FILE--
<?php

class A {
    function NotAConstructor ()
    {
        if (isset($this->x)) {
            //just for demo
        }
    }
}

$t = new A ();

print_r($t);

?>
--EXPECT--
a Object
(
    [x] => 
)
