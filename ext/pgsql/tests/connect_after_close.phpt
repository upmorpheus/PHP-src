--TEST--
Reopen connection after it was closed
--EXTENSIONS--
psql
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
include('config.inc');

/* Run me under valgrind */
$db1 = pg_connect($conn_str);
unset($db1);
var_dump(pg_close());

$db2 = pg_connect($conn_str);
unset($db2);
var_dump(pg_close());
?>
--EXPECT--
bool(true)
bool(true)
