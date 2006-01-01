--TEST--
PHP_Archive::mapPhar truncated manifest (none)
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip";?>
--FILE--
<?php
Phar::mapPhar(5, 'hio', false);
__HALT_COMPILER(); ?>
--EXPECTF--
Fatal error: Phar::mapPhar(): internal corruption of phar "%s" (truncated manifest) in %s on line %d