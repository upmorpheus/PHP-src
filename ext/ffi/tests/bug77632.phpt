--TEST--
Bug #77632 (FFI Segfaults When Called With Variadics)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--INI--
ffi.enable=1
--FILE--
<?php
$libc = FFI::cdef("int printf(const char *format, ...);", "libc.so.6");
$args = ["test\n"];
$libc->printf(...$args);
?>
--EXPECT--
test
