--TEST--
file() on a file with blank lines
--FILE--
<?php

$filepath = __FILE__ . ".tmp";
$fd = fopen($filepath, "w+");
fwrite($fd, "Line 1\n\n \n  \n\Line 3");
fclose($fd);

echo "file():\n";
var_dump(file($filepath));

echo "\nfile() with FILE_IGNORE_NEW_LINES:\n";
var_dump(file($filepath, FILE_IGNORE_NEW_LINES));

echo "\nfile() with FILE_SKIP_EMPTY_LINES:\n";
var_dump(file($filepath, FILE_SKIP_EMPTY_LINES));

echo "\nfile() with FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES:\n";
var_dump(file($filepath, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES));

unlink($filepath);

?>
--EXPECTF--
Notice: fwrite(): 20 character unicode buffer downcoded for binary stream runtime_encoding in %s on line %d
file():
array(5) {
  [0]=>
  string(7) "Line 1
"
  [1]=>
  string(1) "
"
  [2]=>
  string(2) " 
"
  [3]=>
  string(3) "  
"
  [4]=>
  string(7) "\Line 3"
}

file() with FILE_IGNORE_NEW_LINES:
array(5) {
  [0]=>
  string(6) "Line 1"
  [1]=>
  string(0) ""
  [2]=>
  string(1) " "
  [3]=>
  string(2) "  "
  [4]=>
  string(7) "\Line 3"
}

file() with FILE_SKIP_EMPTY_LINES:
array(5) {
  [0]=>
  string(7) "Line 1
"
  [1]=>
  string(1) "
"
  [2]=>
  string(2) " 
"
  [3]=>
  string(3) "  
"
  [4]=>
  string(7) "\Line 3"
}

file() with FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES:
array(4) {
  [0]=>
  string(6) "Line 1"
  [1]=>
  string(1) " "
  [2]=>
  string(2) "  "
  [3]=>
  string(7) "\Line 3"
}
