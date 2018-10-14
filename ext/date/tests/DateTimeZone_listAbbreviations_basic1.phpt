--TEST--
Test DateTimeZone::listAbbreviations() function : basic functionality
--FILE--
<?php
/* Prototype  : array DateTimeZone::listAbbreviations  ( void  )
 * Description: Returns associative array containing dst, offset and the timezone name
 * Source code: ext/date/php_date.c
 * Alias to functions: timezone_abbreviations_list
 */

echo "*** Testing DateTimeZone::listAbbreviations() : basic functionality ***\n";

//Set the default time zone
date_default_timezone_set("GMT");

$abbr = DateTimeZone::listAbbreviations();

var_dump( gettype($abbr) );
var_dump( count($abbr) );

echo "\n-- Format a sample entry --\n";
var_dump( $abbr["acst"] );

?>
===DONE===
--EXPECTF--
*** Testing DateTimeZone::listAbbreviations() : basic functionality ***
string(5) "array"
int(%d)

-- Format a sample entry --
array(12) {
  [0]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(32400)
    ["timezone_id"]=>
    string(18) "Australia/Adelaide"
  }
  [1]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(34200)
    ["timezone_id"]=>
    string(18) "Australia/Adelaide"
  }
  [2]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(32400)
    ["timezone_id"]=>
    string(21) "Australia/Broken_Hill"
  }
  [3]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(32400)
    ["timezone_id"]=>
    string(16) "Australia/Darwin"
  }
  [4]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(32400)
    ["timezone_id"]=>
    string(15) "Australia/North"
  }
  [5]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(32400)
    ["timezone_id"]=>
    string(15) "Australia/South"
  }
  [6]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(32400)
    ["timezone_id"]=>
    string(20) "Australia/Yancowinna"
  }
  [7]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(34200)
    ["timezone_id"]=>
    string(21) "Australia/Broken_Hill"
  }
  [8]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(34200)
    ["timezone_id"]=>
    string(16) "Australia/Darwin"
  }
  [9]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(34200)
    ["timezone_id"]=>
    string(15) "Australia/North"
  }
  [10]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(34200)
    ["timezone_id"]=>
    string(15) "Australia/South"
  }
  [11]=>
  array(3) {
    ["dst"]=>
    bool(false)
    ["offset"]=>
    int(34200)
    ["timezone_id"]=>
    string(20) "Australia/Yancowinna"
  }
}
===DONE===
