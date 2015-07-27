--TEST--
Test dirname() function : usage variations
--FILE--
<?php
/* Prototype: string dirname ( string $path [, int nb]);
   Description: Returns directory name component of path.
*/
for ($i=0 ; $i<5 ; $i++) {
	var_dump(dirname("/foo/bar/baz", $i));
}
var_dump(dirname("/foo/bar/baz", PHP_INT_MAX));
?>
Done
--EXPECTF--
Warning: dirname(): Invalid argument, levels must be >= 1 in %sdirname_multi.php on line %d
NULL
string(8) "/foo/bar"
string(4) "/foo"
string(1) "/"
string(1) "/"
string(1) "/"
Done
