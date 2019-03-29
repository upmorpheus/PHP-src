--TEST--
Bug #69151 (mb_ereg should reject ill-formed byte sequence)
--SKIPIF--
<?php
if (!extension_loaded('mbstring')) die('skip mbstring extension not available');
if (!function_exists('mb_ereg')) die('skip mb_ereg() not available');
?>
--FILE--
<?php
$str = "\x80";
var_dump(
    false === mb_eregi('.', $str, $matches),
    [] === $matches,
    NULL === mb_ereg_replace('.', "\\0", $str),
    false === mb_ereg_search_init("\x80", '.'),
    false === mb_ereg_search()
);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
