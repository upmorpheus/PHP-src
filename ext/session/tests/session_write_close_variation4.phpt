--TEST--
Test session_write_close() function : variation
--EXTENSIONS--
session
--SKIPIF--
<?php include('skipif.inc'); ?>
--INI--
session.use_strict_mode=0
--FILE--
<?php

ob_start();

echo "*** Testing session_write_close() : variation ***\n";

var_dump(session_id("test"));
var_dump(session_start());
var_dump(session_id());
var_dump(session_write_close());
var_dump(session_id());
var_dump(session_start());
var_dump(session_id());
var_dump(session_write_close());
var_dump(session_id());
var_dump(session_start());
var_dump(session_id());
var_dump(session_write_close());
var_dump(session_id());
var_dump(session_start());
var_dump(session_destroy());

echo "Done";
ob_end_flush();
?>
--EXPECT--
*** Testing session_write_close() : variation ***
string(0) ""
bool(true)
string(4) "test"
bool(true)
string(4) "test"
bool(true)
string(4) "test"
bool(true)
string(4) "test"
bool(true)
string(4) "test"
bool(true)
string(4) "test"
bool(true)
bool(true)
Done
