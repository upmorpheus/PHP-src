--TEST--
mysqli_driver properties
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	include "connect.inc";

    var_dump( $driver->embedded);
    var_dump( $driver->client_version);
    var_dump( $driver->client_info);
    var_dump( $driver->driver_version);
	var_dump( $driver->reconnect);
	var_dump( $driver->report_mode);
    
?>
--EXPECTF--
bool(%s)
int(%d)
string(%d) "%s"
int(%d)
bool(%s)
int(%d)
