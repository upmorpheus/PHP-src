--TEST--
Unexposed/leaked stream encloses another stream
--SKIPIF--
<?php
if (!function_exists('zend_leak_variable')) die("skip only debug builds");
--FILE--
<?php
$s = fopen('php://temp/maxmemory=1024','wb+');

$t = fopen('php://temp/maxmemory=1024','wb+');

/* force conversion of inner stream to STDIO. */
$i = 0;
while ($i++ < 5000) {
    fwrite($t, str_repeat('a',1024));
}

zend_leak_variable($s);
zend_leak_variable($t);
--EXPECT--
