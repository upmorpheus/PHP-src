--TEST--
Test session_commit() function : variation
--EXTENSIONS--
session
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

ob_start();

echo "*** Testing session_commit() : variation ***\n";

var_dump(session_start());
var_dump($_SESSION);
var_dump(session_commit());
var_dump($_SESSION);
var_dump(session_start());
var_dump($_SESSION);
var_dump(session_commit());
var_dump($_SESSION);
var_dump(session_start());
var_dump($_SESSION);
var_dump(session_commit());
var_dump($_SESSION);
var_dump(session_start());
var_dump(session_destroy());

echo "Done";
ob_end_flush();
?>
--EXPECT--
*** Testing session_commit() : variation ***
bool(true)
array(0) {
}
bool(true)
array(0) {
}
bool(true)
array(0) {
}
bool(true)
array(0) {
}
bool(true)
array(0) {
}
bool(true)
array(0) {
}
bool(true)
bool(true)
Done
