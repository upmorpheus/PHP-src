--TEST--
pass a string into fromArray()
--CREDITS--
PHPNW Testfest 2009 - Lorna Mitchell
--FILE--
<?php
echo SplFixedArray::fromArray('hello');
?>
--EXPECTF--
Warning: SplFixedArray::fromArray() expects parameter 1 to be array, string given in %s on line %d
