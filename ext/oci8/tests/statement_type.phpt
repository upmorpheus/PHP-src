--TEST--
oci_statement_type()
--SKIPIF--
<?php if (!extension_loaded('oci8')) die("skip no oci8 extension"); ?>
--FILE--
<?php

require dirname(__FILE__)."/connect.inc";

if (!empty($dbase)) {
	var_dump($c = oci_connect($user, $password, $dbase));
}
else {
	var_dump($c = oci_connect($user, $password));
}

$sqls = Array(
	"SELECT * FROM table",
	"DELETE FROM table WHERE id = 1",
	"INSERT INTO table VALUES(1)",
	"UPDATE table SET id = 1",
	"DROP TABLE table",
	"CREATE TABLE table (id NUMBER)",
	"WRONG SYNTAX",
	""
);

foreach ($sqls as $sql) {
	$s = oci_parse($c, $sql);
	var_dump(oci_statement_type($s));
}

echo "Done\n";

?>
--EXPECTF--
resource(%d) of type (oci8 connection)
string(6) "SELECT"
string(6) "DELETE"
string(6) "INSERT"
string(6) "UPDATE"
string(4) "DROP"
string(6) "CREATE"
string(7) "UNKNOWN"
string(7) "UNKNOWN"
Done
--UEXPECTF--
resource(%d) of type (oci8 connection)
unicode(6) "SELECT"
unicode(6) "DELETE"
unicode(6) "INSERT"
unicode(6) "UPDATE"
unicode(4) "DROP"
unicode(6) "CREATE"
unicode(7) "UNKNOWN"
unicode(7) "UNKNOWN"
Done
