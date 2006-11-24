--TEST--
show information about function
--SKIPIF--
<?php include "skipif.inc"; ?>
--FILE--
<?php

$php = $_ENV['TEST_PHP_EXECUTABLE'];

var_dump(`"$php" --rf unknown`);
var_dump(`"$php" --rf echo`);
var_dump(`"$php" --rf phpinfo`);

echo "Done\n";
?>
--EXPECTF--	
string(45) "Exception: Function unknown() does not exist
"
string(42) "Exception: Function echo() does not exist
"
string(119) "Function [ <internal:standard> function phpinfo ] {

  - Parameters [1] {
    Parameter #0 [ <optional> $what ]
  }
}

"
Done
