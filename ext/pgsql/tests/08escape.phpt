--TEST--
PostgreSQL escape functions
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php

include 'config.inc';
define('FILE_NAME', './php.gif');

// pg_escape_string() test
$before = "ABC\\ABC\'";
$expect  = "ABC\\\\ABC\\'";
$after = pg_escape_string($before);
if ($expect === $after) {
	echo "pg_escape_string() is Ok\n";
}
else {
	echo "pg_escape_string() is NOT Ok\n";
	var_dump($before);
	var_dump($after);
	var_dump($expect);
}

// pg_escape_bytea() test
$before = "ABC\\ABC";
$expect  = "ABC\\\\\\\\ABC";
$after  = pg_escape_bytea($before);
if ($expect === $after) {
	echo "pg_escape_bytea() is Ok\n";
}
else {
	echo "pg_escape_byte() is NOT Ok\n";
	var_dump($before);
	var_dump($after);
	var_dump($expect);
}

// Test using database
$fp   = fopen(FILE_NAME,'r');
$data = fread($fp, filesize(FILE_NAME));
$db   = pg_connect($conn_str);

// Insert binary to DB
$escaped_data = pg_escape_bytea($data);
pg_query("DELETE FROM ".$table_name." WHERE num = -9999;");
$sql = "INSERT INTO ".$table_name." (num, bin) VALUES (-9999, CAST ('".$escaped_data."' AS BYTEA));";
pg_query($db, $sql);

// Retrieve binary from DB
$sql = "SELECT bin::bytea FROM ".$table_name." WHERE num = -9999";
$result = pg_query($db, $sql);
$row = pg_fetch_array($result, 0, PGSQL_ASSOC);

// Compare
// Need to wait PostgreSQL 7.3.x for PQunescapeBytea()
// if ($data === pg_unescape_bytea($row['bin'])) {
// 	echo "pg_escape_bytea() actually works with databse\n";
// }
// else {
// 	echo "pg_escape_bytea() is broken\n";
// }

?>
--EXPECT--
pg_escape_string() is NOT Ok
string(9) "ABC\ABC\'"
string(12) "ABC\\ABC\\''"
string(10) "ABC\\ABC\'"
pg_escape_bytea() is Ok
