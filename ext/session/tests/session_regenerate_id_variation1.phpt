--TEST--
Test session_regenerate_id() function : variation
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

ob_start();

echo "*** Testing session_regenerate_id() : variation ***\n";

var_dump(session_id());
var_dump(session_regenerate_id(TRUE));
var_dump(session_id());
var_dump(session_start());
var_dump(session_regenerate_id(TRUE));
var_dump(session_id());
var_dump(session_destroy());
var_dump(session_regenerate_id(TRUE));
var_dump(session_id());

echo "Done";
ob_end_flush();
?>
--EXPECTF--
*** Testing session_regenerate_id() : variation ***
string(0) ""

Warning: session_regenerate_id(): Cannot regenerate session id - session is not active in %s on line %d
bool(false)
string(0) ""
bool(true)
bool(true)
string(%d) "%s"
bool(true)

Warning: session_regenerate_id(): Cannot regenerate session id - session is not active in %s on line %d
bool(false)
string(0) ""
Done
