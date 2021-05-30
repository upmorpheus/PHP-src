--TEST--
Bug #81070	Setting memory limit to below current usage
--FILE--
<?php
$a = str_repeat("0", 5 * 1024 * 1024);
ini_set("memory_limit", "3M");
?>
--EXPECTF--
Warning: Failed to set memory limit to 3145728 bytes (Current memory usage is %d bytes) in %s on line %d
