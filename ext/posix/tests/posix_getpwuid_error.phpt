--TEST--
Test posix_getpwuid() function : error conditions
--SKIPIF--
<?php
if(!extension_loaded("posix")) dir("skip - POSIX extension not loaded");
if (getenv('SKIP_ASAN')) die('skip LSan crashes when firebird is loaded');
?>
--FILE--
<?php
echo "*** Testing posix_getpwuid() : error conditions ***\n";

echo "\n-- Testing posix_getpwuid() function negative uid --\n";
$uid = -99;
var_dump( posix_getpwuid($uid) );

echo "Done";
?>
--EXPECT--
*** Testing posix_getpwuid() : error conditions ***

-- Testing posix_getpwuid() function negative uid --
bool(false)
Done
