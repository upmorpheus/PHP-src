--TEST--
PostgreSQL notice function
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
include 'config.inc';

ini_set('pgsql.log_notice',1);

$db = pg_connect($conn_str);
pg_query($db, "BEGIN;");
pg_query($db, "BEGIN;");

$msg = pg_last_notice($db);
if ($msg === FALSE) {
	echo "Cannot find notice message in hash\n";
	var_dump($msg);
}
echo $msg;
echo "pg_last_notice() is Ok\n";

?>
--EXPECT--
NOTICE:  BEGIN: already a transaction in progress

NOTICE:  BEGIN: already a transaction in progress
pg_last_notice() is Ok
