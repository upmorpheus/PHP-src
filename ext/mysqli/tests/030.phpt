--TEST--
function test: mysqli_errno
--SKIPIF--
<?php 
require_once('skipif.inc'); 
require_once('skipifconnectfailure.inc');
?>
--FILE--
<?php
	include "connect.inc";

	/*** test mysqli_connect 127.0.0.1 ***/
	$link = mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	$errno = mysqli_errno($link);
	var_dump($errno);

	mysqli_select_db($link, "test");

	mysqli_query($link, "select * from non_exisiting_table");
	$errno = mysqli_errno($link);

	var_dump($errno);

	mysqli_close($link);
	print "done!";
?>
--EXPECT--
int(0)
int(1146)
done!