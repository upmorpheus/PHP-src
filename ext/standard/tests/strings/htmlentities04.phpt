--TEST--
htmlentities() test 4 (setlocale / ja_JP.EUC-JP)
--SKIPIF--
<?php setlocale(LC_CTYPE, "ja_JP.EUC-JP", "ja_JP.eucJP") or die("skip setlocale() failed\n"); ?>
--INI--
output_handler=
mbstring.internal_encoding=pass
--FILE--
<?php
	setlocale( LC_CTYPE, "ja_JP.EUC-JP", "ja_JP.eucJP" );
	var_dump(htmlentities("\xa1\xa2\xa1\xa3\xa1\xa4", ENT_QUOTES, ''));
?>
--EXPECT--
string(6) "������"
