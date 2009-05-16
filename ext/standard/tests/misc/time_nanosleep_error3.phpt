--TEST--
time_nanosleep — Delay for a number of seconds and nanoseconds
--SKIPIF--
<?php if (!function_exists('time_nanosleep')) die("skip"); ?>
--CREDITS--
Àlex Corretgé - alex@corretge.cat
--FILE--
<?php

$nano = time_nanosleep(-2, 1000);

?>
--EXPECTF--
Warning: time_nanosleep(): nanoseconds was not in the range 0 to 999 999 999 or seconds was negative in %s.php on line %d
