--TEST--
zend_strtod() leaks on big doubles
--INI--
precision=14
--FILE--
<?php
var_dump("1139932690.21688500" - "1139932790.21688500");
var_dump("1139932690000.21688500" - "331139932790.21688500");
var_dump("339932690.21688500" - "4564645646456463461139932790.21688500");
var_dump("123123139932690.21688500" - "11399327900000000.21688500");

echo "Done\n";
?>
--EXPECT--	
float(-100)
float(808792757210)
float(-4.5646456464565E+27)
float(-1.1276204760067E+16)
Done
