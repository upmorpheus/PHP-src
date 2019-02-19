--TEST--
Test flock() function: Error conditions
--FILE--
<?php
/*
Prototype: bool flock(resource $handle, int $operation [, int &$wouldblock]);
Description: PHP supports a portable way of locking complete files
  in an advisory way
*/

echo "*** Testing error conditions ***\n";

$file = dirname(__FILE__)."/flock.tmp";
$fp = fopen($file, "w");

/* array of operatons */
$operations = array(
  0,
  LOCK_NB,
  FALSE,
  NULL,
  array(1,2,3),
  array(),
  "string",
  "",
  "\0"
);

$i = 0;
foreach($operations as $operation) {
  echo "\n--- Iteration $i ---";
  try {
    var_dump(flock($fp, $operation));
  } catch (TypeError $e) {
    echo "\n", $e->getMessage(), "\n";
  }
  $i++;
}


/* Invalid arguments */
$fp = fopen($file, "w");
fclose($fp);
var_dump(flock($fp, LOCK_SH|LOCK_NB));

echo "\n*** Done ***\n";
?>
--CLEAN--
<?php
$file = dirname(__FILE__)."/flock.tmp";
unlink($file);
?>
--EXPECTF--
*** Testing error conditions ***

--- Iteration 0 ---
Warning: flock(): Illegal operation argument in %s on line %d
bool(false)

--- Iteration 1 ---
Warning: flock(): Illegal operation argument in %s on line %d
bool(false)

--- Iteration 2 ---
Warning: flock(): Illegal operation argument in %s on line %d
bool(false)

--- Iteration 3 ---
Warning: flock(): Illegal operation argument in %s on line %d
bool(false)

--- Iteration 4 ---
flock() expects parameter 2 to be int, array given

--- Iteration 5 ---
flock() expects parameter 2 to be int, array given

--- Iteration 6 ---
flock() expects parameter 2 to be int, string given

--- Iteration 7 ---
flock() expects parameter 2 to be int, string given

--- Iteration 8 ---
flock() expects parameter 2 to be int, string given

Warning: flock(): supplied resource is not a valid stream resource in %s on line %d
bool(false)

*** Done ***
