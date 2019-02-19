--TEST--
mysqli_sqlstate()
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifemb.inc');
require_once('skipifconnectfailure.inc');
?>
--FILE--
<?php
	require_once("connect.inc");

	require('table.inc');

	var_dump(mysqli_sqlstate($link));
	mysqli_query($link, "SELECT unknown_column FROM test");
	var_dump(mysqli_sqlstate($link));
	mysqli_free_result(mysqli_query($link, "SELECT id FROM test"));
	var_dump(mysqli_sqlstate($link));

	mysqli_close($link);

	var_dump(mysqli_sqlstate($link));

	print "done!";
?>
--CLEAN--
<?php
	require_once("clean_table.inc");
?>
--EXPECTF--
%s(5) "00000"
%s(5) "42S22"
%s(5) "00000"

Warning: mysqli_sqlstate(): Couldn't fetch mysqli in %s on line %d
bool(false)
done!
