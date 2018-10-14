--TEST--
Test deflate_add() errors with ZLIB_BLOCK in zlib < 1.2.4
--SKIPIF--
<?php
if (!extension_loaded("zlib")) {
    print "skip - ZLIB extension not loaded";
}
if (ZLIB_VERNUM >= 0x1240) {
    print "skip - ZLIB < 1.2.4 required for test";
}
?>
--FILE--
<?php

$resource = deflate_init(ZLIB_ENCODING_GZIP);
var_dump(deflate_add($resource, "aaaaaaaaaaaaaaaaaaaaaa", ZLIB_BLOCK));

?>
===DONE===
--EXPECTF--

Warning: deflate_add(): zlib >= 1.2.4 required for BLOCK deflate; current version: %s in %s on line %d
bool(false)
===DONE===
