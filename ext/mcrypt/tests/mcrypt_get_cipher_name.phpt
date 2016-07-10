--TEST--
mcrypt_get_cipher_name
--SKIPIF--
<?php if (!extension_loaded("mcrypt")) print "skip"; ?>
--FILE--
<?php
echo mcrypt_get_cipher_name(MCRYPT_RIJNDAEL_256) . "\n";
echo mcrypt_get_cipher_name(MCRYPT_RC2) . "\n";
echo mcrypt_get_cipher_name(MCRYPT_ARCFOUR) . "\n";
echo mcrypt_get_cipher_name(MCRYPT_WAKE) . "\n";
--EXPECTF--
Deprecated: mcrypt_get_cipher_name(): The mcrypt extension is deprecated and will be removed in the future: use openssl instead in %s%emcrypt_get_cipher_name.php on line 2
Rijndael-256

Deprecated: mcrypt_get_cipher_name(): The mcrypt extension is deprecated and will be removed in the future: use openssl instead in %s%emcrypt_get_cipher_name.php on line 3
RC2

Deprecated: mcrypt_get_cipher_name(): The mcrypt extension is deprecated and will be removed in the future: use openssl instead in %s%emcrypt_get_cipher_name.php on line 4
RC4

Deprecated: mcrypt_get_cipher_name(): The mcrypt extension is deprecated and will be removed in the future: use openssl instead in %s%emcrypt_get_cipher_name.php on line 5
WAKE
