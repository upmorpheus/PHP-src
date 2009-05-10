--TEST--
zend multibyte (9)
--SKIPIF--
<?php
ini_set("mbstring.script_encoding","SJIS");
if (ini_set("mbstring.script_encoding","SJIS") != "SJIS") {
	die("skip zend-multibyte is not available");
}
?>
--INI--
mbstring.script_encoding=cp1251
mbstring.internal_encoding=UTF-8
--FILE--
<?php
declare(encoding="EUC-JP");
// forcefully interpret an UTF-8 encoded string as EUC-JP and then convert it
// back to UTF-8. There should be only a pair of consecutive bytes that is
// valid EUC-encoded character "鴻".
var_dump(bin2hex("テスト"));
?>
--EXPECT--
string(16) "3f3f3fe9b4bb3f3f"
