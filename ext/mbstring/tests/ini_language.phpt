--TEST--
mbstring.language bug
--SKIPIF--
<?php extension_loaded('mbstring') or die('skip mbstring not available'); ?>
--INI--
internal_encoding=Shift_JIS
mbstring.language=Japanese
--FILE--
<?php
var_dump(ini_get('internal_encoding'));
var_dump(mb_internal_encoding());
?>
--EXPECT--
string(9) "Shift_JIS"
string(4) "SJIS"
