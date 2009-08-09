--TEST--
Test socket_set_block return values
--SKIPIF--
<?php
if (!extension_loaded('sockets')) {
    die('SKIP The sockets extension is not loaded.');
}
?>
--FILE--
<?php

$socket = socket_create_listen(31339);
var_dump(socket_set_block($socket));
socket_close($socket);

$socket2 = socket_create_listen(31340);
socket_close($socket2);
var_dump(socket_set_block($socket2));

?>
--EXPECTF--
bool(true)

Warning: socket_set_block(): %d is not a valid Socket resource in %s on line %d
bool(false)
--CREDITS--
Robin Mehner, robin@coding-robin.de
PHP Testfest Berlin 2009-05-09
