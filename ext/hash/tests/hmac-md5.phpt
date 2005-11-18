--TEST--
hmac-md5 algorithm
--SKIPIF--
<?php if(!extension_loaded("hash")) print "skip"; ?>
--FILE--
<?php
/* Test Vectors from RFC 2104 */
$ctx = hash_init('md5',HASH_HMAC,str_repeat(chr(0x0b), 16));
hash_update($ctx, 'Hi There');
echo hash_final($ctx) . "\n";

$ctx = hash_init('md5',HASH_HMAC,'Jefe');
hash_update($ctx, 'what do ya want for nothing?');
echo hash_final($ctx) . "\n";

$ctx = hash_init('md5',HASH_HMAC,str_repeat(chr(0xAA), 16));
hash_update($ctx, str_repeat(chr(0xDD), 50));
echo hash_final($ctx) . "\n";
--EXPECTF--
9294727a3638bb1c13f48ef8158bfc9d
750c783e6ab0b503eaa86e310a5db738
56be34521d144c88dbb8c733f0e8b3f6
