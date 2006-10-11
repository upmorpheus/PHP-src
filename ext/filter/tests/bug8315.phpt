--TEST--
bug 8315, NULL values halt the validation 
--FILE--
<?php

$var="3".chr(0)."foo";
var_dump(filter_var($var, FILTER_VALIDATE_INT));
$var="3".chr(0)."foo";
var_dump(filter_var($var, FILTER_VALIDATE_FLOAT));
?>
--EXPECTF--	
NULL
NULL
