--TEST--
Test chdir() function : error conditions - Non-existent directory
--FILE--
<?php
/* Prototype  : bool chdir(string $directory)
 * Description: Change the current directory
 * Source code: ext/standard/dir.c
 */

/*
 * Pass a directory that does not exist as $directory to chdir() to test behaviour
 */

echo "*** Testing chdir() : error conditions ***\n";

$directory = __FILE__ . '/idonotexist';

var_dump(chdir($directory));
?>
--EXPECTF--
*** Testing chdir() : error conditions ***

Warning: chdir(): %s (errno %d) in %s on line %d
bool(false)
