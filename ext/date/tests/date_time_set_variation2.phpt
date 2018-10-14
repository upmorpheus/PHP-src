--TEST--
Test date_time_set() function : usage variation - Passing unexpected values to second argument $hour.
--FILE--
<?php
/* Prototype  : DateTime date_time_set  ( DateTime $object  , int $hour  , int $minute  [, int $second  ] )
 * Description: Resets the current time of the DateTime object to a different time.
 * Source code: ext/date/php_date.c
 * Alias to functions: DateTime::setTime
 */

echo "*** Testing date_time_set() : usage variation -  unexpected values to second argument \$hour***\n";

//Set the default time zone
date_default_timezone_set("Europe/London");

//get an unset variable
$unset_var = 10;
unset ($unset_var);

// define some classes
class classWithToString
{
	public function __toString() {
		return "Class A object";
	}
}

class classWithoutToString
{
}

// heredoc string
$heredoc = <<<EOT
hello world
EOT;

// add arrays
$index_array = array (1, 2, 3);
$assoc_array = array ('one' => 1, 'two' => 2);

// resource
$file_handle = fopen(__FILE__, 'r');

//array of values to iterate over
$inputs = array(

      // int data
      'int 0' => 0,
      'int 1' => 1,
      'int 12345' => 12345,
      'int -12345' => -12345,

      // float data
      'float 10.5' => 10.5,
      'float -10.5' => -10.5,
      'float .5' => .5,

      // array data
      'empty array' => array(),
      'int indexed array' => $index_array,
      'associative array' => $assoc_array,
      'nested arrays' => array('foo', $index_array, $assoc_array),

      // null data
      'uppercase NULL' => NULL,
      'lowercase null' => null,

      // boolean data
      'lowercase true' => true,
      'lowercase false' =>false,
      'uppercase TRUE' =>TRUE,
      'uppercase FALSE' =>FALSE,

      // empty data
      'empty string DQ' => "",
      'empty string SQ' => '',

      // string data
      'string DQ' => "string",
      'string SQ' => 'string',
      'mixed case string' => "sTrInG",
      'heredoc' => $heredoc,

      // object data
      'instance of classWithToString' => new classWithToString(),
      'instance of classWithoutToString' => new classWithoutToString(),

      // undefined data
      'undefined var' => @$undefined_var,

      // unset data
      'unset var' => @$unset_var,

      // resource
      'resource' => $file_handle
);

$object = date_create("2009-01-31 15:14:10");
$minute = 13;
$sec = 45;

foreach($inputs as $variation =>$hour) {
      echo "\n-- $variation --\n";
      var_dump( date_time_set($object, $hour, $minute, $sec) );
};

// closing the resource
fclose( $file_handle );

?>
===DONE===
--EXPECTF--
*** Testing date_time_set() : usage variation -  unexpected values to second argument $hour***

-- int 0 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-31 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- int 1 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-31 01:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- int 12345 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2010-06-29 09:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- int -12345 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-30 15:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- float 10.5 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-30 10:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- float -10.5 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 14:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- float .5 --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- empty array --

Warning: date_time_set() expects parameter 2 to be int, array given in %s
bool(false)

-- int indexed array --

Warning: date_time_set() expects parameter 2 to be int, array given in %s
bool(false)

-- associative array --

Warning: date_time_set() expects parameter 2 to be int, array given in %s
bool(false)

-- nested arrays --

Warning: date_time_set() expects parameter 2 to be int, array given in %s
bool(false)

-- uppercase NULL --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- lowercase null --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- lowercase true --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 01:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- lowercase false --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- uppercase TRUE --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 01:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- uppercase FALSE --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- empty string DQ --

Warning: date_time_set() expects parameter 2 to be int, string given in %s
bool(false)

-- empty string SQ --

Warning: date_time_set() expects parameter 2 to be int, string given in %s
bool(false)

-- string DQ --

Warning: date_time_set() expects parameter 2 to be int, string given in %s
bool(false)

-- string SQ --

Warning: date_time_set() expects parameter 2 to be int, string given in %s
bool(false)

-- mixed case string --

Warning: date_time_set() expects parameter 2 to be int, string given in %s
bool(false)

-- heredoc --

Warning: date_time_set() expects parameter 2 to be int, string given in %s
bool(false)

-- instance of classWithToString --

Warning: date_time_set() expects parameter 2 to be int, object given in %s
bool(false)

-- instance of classWithoutToString --

Warning: date_time_set() expects parameter 2 to be int, object given in %s
bool(false)

-- undefined var --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- unset var --
object(DateTime)#%d (3) {
  ["date"]=>
  string(26) "2009-01-29 00:13:45.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}

-- resource --

Warning: date_time_set() expects parameter 2 to be int, resource given in %s
bool(false)
===DONE===
