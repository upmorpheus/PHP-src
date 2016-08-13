--TEST--
mcrypt_get_block_size
--SKIPIF--
<?php if (!extension_loaded("mcrypt")) print "skip"; ?>
--FILE--
<?php
var_dump(mcrypt_get_block_size(MCRYPT_RIJNDAEL_256, MCRYPT_MODE_CBC));
var_dump(mcrypt_get_block_size(MCRYPT_3DES, MCRYPT_MODE_CBC));
var_dump(mcrypt_get_block_size(MCRYPT_WAKE, MCRYPT_MODE_STREAM));
--EXPECTF--
Deprecated: Function mcrypt_get_block_size() is deprecated in %s%emcrypt_get_block_size.php on line 2
int(32)

Deprecated: Function mcrypt_get_block_size() is deprecated in %s%emcrypt_get_block_size.php on line 3
int(8)

Deprecated: Function mcrypt_get_block_size() is deprecated in %s%emcrypt_get_block_size.php on line 4
int(1)
