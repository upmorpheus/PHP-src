--TEST--
Bug #72710 (`mb_ereg` causes buffer overflow on regexp compile error)
--SKIPIF--
<?php
if (!extension_loaded('mbstring')) die('skip ext/mbstring required');
if (!function_exists('mb_ereg')) die('skip mbregex support not available');
?>
--FILE--
<?php
mb_ereg('(?<0>a)', 'a');
?>
--EXPECTF--
Warning: mb_ereg(): mbregex compile err: invalid group name <0> in %s on line %d
