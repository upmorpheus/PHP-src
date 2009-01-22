--TEST--
Test gmstrftime() function : usage variation - Checking week related formats which are not supported on Windows. 
--SKIPIF--
<?php
if (strtoupper(substr(PHP_OS, 0, 3)) != 'WIN') {
    die("skip Test is valid for Windows");
}
?>
--FILE--
<?php
/* Prototype  : string gmstrftime(string format [, int timestamp])
 * Description: Format a GMT/UCT time/date according to locale settings 
 * Source code: ext/date/php_date.c
 * Alias to functions: 
 */

echo "*** Testing gmstrftime() : usage variation ***\n";

// Initialise function arguments not being substituted (if any)
$timestamp = gmmktime(8, 8, 8, 8, 8, 2008);
locale_set_default("en_US");
date_default_timezone_set("Asia/Calcutta");

//array of values to iterate over
$inputs = array(
	  'The ISO 8601:1988 week number' => "%V",
	  'Weekday as decimal' => "%u",
);

// loop through each element of the array for timestamp

foreach($inputs as $key =>$value) {
      echo "\n--$key--\n";
      var_dump( gmstrftime($value) );
      var_dump( gmstrftime($value, $timestamp) );
};

?>
===DONE===
--EXPECTF--
*** Testing gmstrftime() : usage variation ***

--The ISO 8601:1988 week number--
bool(false)
bool(false)

--Weekday as decimal--
bool(false)
bool(false)
===DONE===
