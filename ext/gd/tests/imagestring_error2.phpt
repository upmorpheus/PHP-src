--TEST--
Testing error on non-image resource parameter 1 of imagestring() of GD library
--CREDITS--
Rafael Dohms <rdohms [at] gmail [dot] com>
#testfest PHPSP on 2009-06-20
--SKIPIF--
<?php
	if (!extension_loaded("gd")) die("skip GD not present");
?>
--FILE--
<?php

$result = imagestring(tmpfile(), 1, 5, 5, 'String', 1);

?>
--EXPECTF--
Warning: imagestring(): supplied resource is not a valid Image resource in %s on line %d
