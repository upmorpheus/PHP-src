--TEST--
Bug #53226 (file_exists fails on big filenames)
--INI--
open_basedir=.
--FILE--
<?php
require_once "open_basedir.inc";
create_directories();

var_dump(file_exists('./test/ok/ok.txt'));
var_dump(file_exists('./test/foo'));

// Picked an arbitrarily large number that should be beyond PATH_MAX on every
// OS I know about.
$file = str_repeat('x', 40000);
var_dump(file_exists("./test/$file"));
?>
--CLEAN--
<?php
require_once "open_basedir.inc";
delete_directories();
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(true)
bool(false)

Warning: file_exists(): File name is longer than the maximum allowed path length on this platform (%d): %s in %s on line %d
bool(false)
