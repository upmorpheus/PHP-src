--TEST--
zend multibyte (10)
--SKIPIF--
--FILE--
<?php
declare(encoding="ISO-8859-15");
declare(encoding="ISO-8859-1");
echo "ok\n";
?>
--EXPECTF--
ok
