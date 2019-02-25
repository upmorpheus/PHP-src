--TEST--
socket_export_stream: effects of leaked handles
--SKIPIF--
<?php
if (!extension_loaded('sockets')) {
	die('SKIP sockets extension not available.');
}
if (!function_exists('zend_leak_variable'))
	die('SKIP only for debug builds');
--FILE--
<?php

$sock0 = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_bind($sock0, '0.0.0.0', 58385);
$stream0 = socket_export_stream($sock0);
zend_leak_variable($stream0);

$sock1 = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_bind($sock1, '0.0.0.0', 58386);
$stream1 = socket_export_stream($sock1);
zend_leak_variable($sock1);

echo "Done.\n";
--EXPECT--
Done.
