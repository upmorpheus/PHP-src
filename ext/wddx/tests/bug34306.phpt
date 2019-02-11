--TEST--
#34306 (wddx_serialize_value() crashes with long array keys)
--SKIPIF--
<?php if (!extension_loaded("wddx")) print "skip"; ?>
--FILE--
<?php

$var = array('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa12345678901234567890123456789012345678901234567890ba12345678901234567890123456789012345678901234567890ba12345678901234567890123456789012345678901234567890ba12345678901234567890123456789012345678901234567890b12345678901234567891234567890123123121231211111' => 1);
$buf = wddx_serialize_value($var, 'name');
echo "OK\n";

?>
--EXPECTF--
Deprecated: Function wddx_serialize_value() is deprecated in %s on line %d
OK
