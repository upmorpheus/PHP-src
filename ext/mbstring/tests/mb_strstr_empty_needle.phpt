--TEST--
Test mb_strstr() function : with empty needle
--EXTENSIONS--
mbstring
--SKIPIF--
<?php
function_exists('mb_strstr') or die("skip mb_strstr() is not available in this build");
?>
--FILE--
<?php

mb_internal_encoding('UTF-8');

$string_ascii = 'abc def';
// Japanese string in UTF-8
$string_mb = "日本語テキストです。01234５６７８９。";

echo "\n-- ASCII string --\n";
var_dump(mb_strstr($string_ascii, '', false, 'ISO-8859-1'));
var_dump(mb_strstr($string_ascii, ''));
var_dump(mb_strstr($string_ascii, '', true));

echo "\n-- Multibyte string --\n";
var_dump(mb_strstr($string_mb, ''));
var_dump(mb_strstr($string_mb, '', false, 'utf-8'));
var_dump(mb_strstr($string_mb, '', true));

?>
--EXPECT--
-- ASCII string --
string(7) "abc def"
string(7) "abc def"
string(0) ""

-- Multibyte string --
string(53) "日本語テキストです。01234５６７８９。"
string(53) "日本語テキストです。01234５６７８９。"
string(0) ""
