--TEST--
filter_data() & FILTER_VALIDATE_IP and weird data
--FILE--
<?php

var_dump(filter_data("....", FILTER_VALIDATE_IP));
var_dump(filter_data("...", FILTER_VALIDATE_IP));
var_dump(filter_data("..", FILTER_VALIDATE_IP));
var_dump(filter_data(".", FILTER_VALIDATE_IP));
var_dump(filter_data("1.1.1.1", FILTER_VALIDATE_IP));

echo "Done\n";
?>
--EXPECTF--	
NULL
NULL
NULL
NULL
string(7) "1.1.1.1"
Done
