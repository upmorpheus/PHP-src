--TEST--
bcsub() incorrect argument count
--SKIPIF--
<?php if(!extension_loaded("bcmath")) print "skip"; ?>
--INI--
bcmath.scale=0
--FILE--
<?php
echo bcsub();
?>
--EXPECTF--
Warning: bcsub() expects at least 2 parameters, 0 given in %s on line %d
