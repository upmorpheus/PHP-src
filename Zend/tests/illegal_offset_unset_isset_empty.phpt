--TEST--
Using unset(), isset(), empty() with an illegal array offset throws
--FILE--
<?php

$ary = [];
try {
    unset($ary[[]]);
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
try {
    isset($ary[[]]);
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}
try {
    empty($ary[[]]);
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECT--
Cannot access offset of type array in unset
Cannot access offset of type array in isset or empty
Cannot access offset of type array in isset or empty
