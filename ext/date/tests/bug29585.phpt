--TEST--
Bug #29585 (Support week numbers in strtotime())
--FILE--
<?php
echo gmdate("Y-m-d H:i:s", strtotime("2004W30"));

?>
--EXPECT--
2004-07-19 00:00:00
