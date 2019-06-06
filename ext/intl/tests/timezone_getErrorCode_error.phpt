--TEST--
IntlTimeZone::getErrorCode(): errors
--SKIPIF--
<?php
if (!extension_loaded('intl'))
	die('skip intl extension not enabled');
--FILE--
<?php
ini_set("intl.error_level", E_WARNING);

var_dump(intltz_get_error_code(null));
?>
--EXPECTF--
Fatal error: Uncaught TypeError: intltz_get_error_code() expects parameter 1 to be IntlTimeZone, null given in %s:%d
Stack trace:
#0 %s(%d): intltz_get_error_code(NULL)
#1 {main}
  thrown in %s on line %d
