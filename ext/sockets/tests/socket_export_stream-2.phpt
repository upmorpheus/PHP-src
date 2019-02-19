--TEST--
socket_export_stream: Bad arguments
--SKIPIF--
<?php
if (!extension_loaded('sockets')) {
	die('SKIP sockets extension not available.');
}
--FILE--
<?php

var_dump(socket_export_stream(fopen(__FILE__, "rb")));
var_dump(socket_export_stream(stream_socket_server("udp://127.0.0.1:58392", $errno, $errstr, STREAM_SERVER_BIND)));
$s = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
var_dump($s);
socket_close($s);
var_dump(socket_export_stream($s));


echo "Done.";
?>
--EXPECTF--
Warning: socket_export_stream(): supplied resource is not a valid Socket resource in %s on line %d
bool(false)

Warning: socket_export_stream(): supplied resource is not a valid Socket resource in %s on line %d
bool(false)
resource(%d) of type (Socket)

Warning: socket_export_stream(): supplied resource is not a valid Socket resource in %s on line %d
bool(false)
Done.
