--TEST--
htmlentities() test 14 (default_charset / Shift_JIS)
--INI--
output_handler=
mbstring.internal_encoding=pass
default_charset=Shift_JIS
--FILE--
<?php
	print ini_get('default_charset')."\n";
	var_dump(htmlentities("\x81\x41\x81\x42\x81\x43", ENT_QUOTES, ''));
?>
--EXPECTF--
Shift_JIS

Strict Standards: htmlentities(): Only basic entities substitution is supported for multi-byte encodings other than UTF-8; functionality is equivalent to htmlspecialchars in %s on line %d
string(6) "�A�B�C"
