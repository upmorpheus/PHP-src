--TEST--
socket_export_stream: effects of leaked handles
--EXTENSIONS--
sockets
zend_test
--INI--
report_memleaks=0
--FILE--
<?php

$sock0 = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_bind($sock0, '0.0.0.0', 0);
$stream0 = socket_export_stream($sock0);
zend_leak_variable($stream0);

$sock1 = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_bind($sock1, '0.0.0.0', 0);
$stream1 = socket_export_stream($sock1);
zend_leak_variable($sock1);

echo "Done.\n";
?>
--EXPECT--
Done.
