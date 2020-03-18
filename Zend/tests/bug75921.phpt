--TEST--
Bug #75921: Inconsistent error when creating stdObject from empty variable
--FILE--
<?php

try {
    $null->a = 42;
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
var_dump($null);
unset($null);

try {
    $null->a['hello'] = 42;
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
var_dump($null);
unset($null);

try {
    $null->a->b = 42;
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
var_dump($null);
unset($null);

try {
    $null->a['hello']->b = 42;
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
var_dump($null);
unset($null);

try {
    $null->a->b['hello'] = 42;
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
var_dump($null);
unset($null);

?>
--EXPECTF--
Attempt to assign property 'a' of non-object

Warning: Undefined variable: null in %s on line %d
NULL
Attempt to modify property 'a' of non-object

Warning: Undefined variable: null in %s on line %d
NULL

Warning: Undefined variable: null in %s on line %d

Warning: Trying to get property 'a' of non-object in %s on line %d
Attempt to assign property 'b' of non-object

Warning: Undefined variable: null in %s on line %d
NULL

Warning: Undefined variable: null in %s on line %d

Warning: Trying to get property 'a' of non-object in %s on line %d

Warning: Trying to access array offset on value of type null in %s on line %d
Attempt to assign property 'b' of non-object

Warning: Undefined variable: null in %s on line %d
NULL

Warning: Undefined variable: null in %s on line %d

Warning: Trying to get property 'a' of non-object in %s on line %d
Attempt to modify property 'b' of non-object

Warning: Undefined variable: null in %s on line %d
NULL
