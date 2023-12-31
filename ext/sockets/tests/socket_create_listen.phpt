--TEST--
Test if socket binds on 31338
--EXTENSIONS--
sockets
--SKIPIF--
<?php
if (substr(PHP_OS, 0, 3) == 'WIN') {
    die('skip.. Not valid for Windows');
}
?>
--FILE--
<?php
$sock = socket_create_listen(31338);
socket_getsockname($sock, $addr, $port);
var_dump($addr, $port);
?>
--EXPECT--
string(7) "0.0.0.0"
int(31338)
--CREDITS--
Till Klampaeckel, till@php.net
PHP Testfest Berlin 2009-05-09
