--TEST--
Test gettype() & settype() functions : basic functionalities
--INI--
precision=14
--FILE--
<?php
/* Prototype: string gettype ( mixed $var );
   Description: Returns the type of the PHP variable var

   Prototype: bool settype ( mixed &$var, string $type );
   Description: Set the type of variable var to type 
*/

/* Test the basic functionalities of settype() & gettype() functions.
   Use the gettype() to get the type of regular data and use settype()
   to change its type to other types */

/* function to handle catchable errors */
function foo($errno, $errstr, $errfile, $errline) {
//	var_dump($errstr);
   // print error no and error string
   echo "$errno: $errstr\n";
}
//set the error handler, this is required as
// settype() would fail with catachable fatal error 
set_error_handler("foo"); 

echo "**** Testing gettype() and settype() functions ****\n";

$fp = fopen(__FILE__, "r");
$dfp = opendir( dirname(__FILE__) );

$var1 = "another string";
$var2 = array(2,3,4);

class point
{
  var $x;
  var $y;

  function point($x, $y) {
     $this->x = $x;
     $this->y = $y;
  }

  function __toString() {
     return "Object";
  }
}

$unset_var = 10;
unset($unset_var);

$values = array(
  array(1,2,3),
  $var1,
  $var2,
  1,
  -20,
  2.54,
  -2.54,
  NULL,
  false,
  "some string",
  'string',
  b"some string",
  b'string',
  $fp,
  $dfp,
  new point(10,20)
);

$types = array(
  "null",
  "integer",
  "int",
  "float",
  "double",
  "boolean",
  "bool",
  "resource",
  "array",
  "object",
  "string"
);

echo "\n*** Testing gettype(): basic operations ***\n";
foreach ($values as $value) {
  var_dump( gettype($value) );
}

echo "\n*** Testing settype(): basic operations ***\n";
foreach ($types as $type) {
  echo "\n-- Setting type of data to $type --\n"; 
  $loop_count = 1;
  foreach ($values as $var) {
     echo "-- Iteration $loop_count --\n"; $loop_count ++;
     // set to new type
     var_dump( settype($var, $type) );
    
     // dump the var 
     var_dump( $var );
  
     // check the new type 
     var_dump( gettype($var) );  
  }
}

echo "Done\n";
?>
--EXPECTF--	
**** Testing gettype() and settype() functions ****

*** Testing gettype(): basic operations ***
string(5) "array"
string(6) "string"
string(5) "array"
string(7) "integer"
string(7) "integer"
string(6) "double"
string(6) "double"
string(4) "NULL"
string(7) "boolean"
string(6) "string"
string(6) "string"
string(6) "string"
string(6) "string"
string(8) "resource"
string(8) "resource"
string(6) "object"

*** Testing settype(): basic operations ***

-- Setting type of data to null --
-- Iteration 1 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 2 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 3 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 4 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 5 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 6 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 7 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 8 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 9 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 10 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 11 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 12 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 13 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 14 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 15 --
bool(true)
NULL
string(4) "NULL"
-- Iteration 16 --
bool(true)
NULL
string(4) "NULL"

-- Setting type of data to integer --
-- Iteration 1 --
bool(true)
int(1)
string(7) "integer"
-- Iteration 2 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 3 --
bool(true)
int(1)
string(7) "integer"
-- Iteration 4 --
bool(true)
int(1)
string(7) "integer"
-- Iteration 5 --
bool(true)
int(-20)
string(7) "integer"
-- Iteration 6 --
bool(true)
int(2)
string(7) "integer"
-- Iteration 7 --
bool(true)
int(-2)
string(7) "integer"
-- Iteration 8 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 9 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 10 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 11 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 12 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 13 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 14 --
bool(true)
int(5)
string(7) "integer"
-- Iteration 15 --
bool(true)
int(6)
string(7) "integer"
-- Iteration 16 --
8: Object of class point could not be converted to int
bool(true)
int(1)
string(7) "integer"

-- Setting type of data to int --
-- Iteration 1 --
bool(true)
int(1)
string(7) "integer"
-- Iteration 2 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 3 --
bool(true)
int(1)
string(7) "integer"
-- Iteration 4 --
bool(true)
int(1)
string(7) "integer"
-- Iteration 5 --
bool(true)
int(-20)
string(7) "integer"
-- Iteration 6 --
bool(true)
int(2)
string(7) "integer"
-- Iteration 7 --
bool(true)
int(-2)
string(7) "integer"
-- Iteration 8 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 9 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 10 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 11 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 12 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 13 --
bool(true)
int(0)
string(7) "integer"
-- Iteration 14 --
bool(true)
int(5)
string(7) "integer"
-- Iteration 15 --
bool(true)
int(6)
string(7) "integer"
-- Iteration 16 --
8: Object of class point could not be converted to int
bool(true)
int(1)
string(7) "integer"

-- Setting type of data to float --
-- Iteration 1 --
bool(true)
float(1)
string(6) "double"
-- Iteration 2 --
bool(true)
float(0)
string(6) "double"
-- Iteration 3 --
bool(true)
float(1)
string(6) "double"
-- Iteration 4 --
bool(true)
float(1)
string(6) "double"
-- Iteration 5 --
bool(true)
float(-20)
string(6) "double"
-- Iteration 6 --
bool(true)
float(2.54)
string(6) "double"
-- Iteration 7 --
bool(true)
float(-2.54)
string(6) "double"
-- Iteration 8 --
bool(true)
float(0)
string(6) "double"
-- Iteration 9 --
bool(true)
float(0)
string(6) "double"
-- Iteration 10 --
bool(true)
float(0)
string(6) "double"
-- Iteration 11 --
bool(true)
float(0)
string(6) "double"
-- Iteration 12 --
bool(true)
float(0)
string(6) "double"
-- Iteration 13 --
bool(true)
float(0)
string(6) "double"
-- Iteration 14 --
bool(true)
float(5)
string(6) "double"
-- Iteration 15 --
bool(true)
float(6)
string(6) "double"
-- Iteration 16 --
8: Object of class point could not be converted to double
bool(true)
float(1)
string(6) "double"

-- Setting type of data to double --
-- Iteration 1 --
bool(true)
float(1)
string(6) "double"
-- Iteration 2 --
bool(true)
float(0)
string(6) "double"
-- Iteration 3 --
bool(true)
float(1)
string(6) "double"
-- Iteration 4 --
bool(true)
float(1)
string(6) "double"
-- Iteration 5 --
bool(true)
float(-20)
string(6) "double"
-- Iteration 6 --
bool(true)
float(2.54)
string(6) "double"
-- Iteration 7 --
bool(true)
float(-2.54)
string(6) "double"
-- Iteration 8 --
bool(true)
float(0)
string(6) "double"
-- Iteration 9 --
bool(true)
float(0)
string(6) "double"
-- Iteration 10 --
bool(true)
float(0)
string(6) "double"
-- Iteration 11 --
bool(true)
float(0)
string(6) "double"
-- Iteration 12 --
bool(true)
float(0)
string(6) "double"
-- Iteration 13 --
bool(true)
float(0)
string(6) "double"
-- Iteration 14 --
bool(true)
float(5)
string(6) "double"
-- Iteration 15 --
bool(true)
float(6)
string(6) "double"
-- Iteration 16 --
8: Object of class point could not be converted to double
bool(true)
float(1)
string(6) "double"

-- Setting type of data to boolean --
-- Iteration 1 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 2 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 3 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 4 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 5 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 6 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 7 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 8 --
bool(true)
bool(false)
string(7) "boolean"
-- Iteration 9 --
bool(true)
bool(false)
string(7) "boolean"
-- Iteration 10 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 11 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 12 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 13 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 14 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 15 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 16 --
bool(true)
bool(true)
string(7) "boolean"

-- Setting type of data to bool --
-- Iteration 1 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 2 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 3 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 4 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 5 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 6 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 7 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 8 --
bool(true)
bool(false)
string(7) "boolean"
-- Iteration 9 --
bool(true)
bool(false)
string(7) "boolean"
-- Iteration 10 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 11 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 12 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 13 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 14 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 15 --
bool(true)
bool(true)
string(7) "boolean"
-- Iteration 16 --
bool(true)
bool(true)
string(7) "boolean"

-- Setting type of data to resource --
-- Iteration 1 --
2: settype(): Cannot convert to resource type
bool(false)
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(5) "array"
-- Iteration 2 --
2: settype(): Cannot convert to resource type
bool(false)
string(14) "another string"
string(6) "string"
-- Iteration 3 --
2: settype(): Cannot convert to resource type
bool(false)
array(3) {
  [0]=>
  int(2)
  [1]=>
  int(3)
  [2]=>
  int(4)
}
string(5) "array"
-- Iteration 4 --
2: settype(): Cannot convert to resource type
bool(false)
int(1)
string(7) "integer"
-- Iteration 5 --
2: settype(): Cannot convert to resource type
bool(false)
int(-20)
string(7) "integer"
-- Iteration 6 --
2: settype(): Cannot convert to resource type
bool(false)
float(2.54)
string(6) "double"
-- Iteration 7 --
2: settype(): Cannot convert to resource type
bool(false)
float(-2.54)
string(6) "double"
-- Iteration 8 --
2: settype(): Cannot convert to resource type
bool(false)
NULL
string(4) "NULL"
-- Iteration 9 --
2: settype(): Cannot convert to resource type
bool(false)
bool(false)
string(7) "boolean"
-- Iteration 10 --
2: settype(): Cannot convert to resource type
bool(false)
string(11) "some string"
string(6) "string"
-- Iteration 11 --
2: settype(): Cannot convert to resource type
bool(false)
string(6) "string"
string(6) "string"
-- Iteration 12 --
2: settype(): Cannot convert to resource type
bool(false)
string(11) "some string"
string(6) "string"
-- Iteration 13 --
2: settype(): Cannot convert to resource type
bool(false)
string(6) "string"
string(6) "string"
-- Iteration 14 --
2: settype(): Cannot convert to resource type
bool(false)
resource(5) of type (stream)
string(8) "resource"
-- Iteration 15 --
2: settype(): Cannot convert to resource type
bool(false)
resource(6) of type (stream)
string(8) "resource"
-- Iteration 16 --
2: settype(): Cannot convert to resource type
bool(false)
object(point)#1 (2) {
  ["x"]=>
  int(10)
  ["y"]=>
  int(20)
}
string(6) "object"

-- Setting type of data to array --
-- Iteration 1 --
bool(true)
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(5) "array"
-- Iteration 2 --
bool(true)
array(1) {
  [0]=>
  string(14) "another string"
}
string(5) "array"
-- Iteration 3 --
bool(true)
array(3) {
  [0]=>
  int(2)
  [1]=>
  int(3)
  [2]=>
  int(4)
}
string(5) "array"
-- Iteration 4 --
bool(true)
array(1) {
  [0]=>
  int(1)
}
string(5) "array"
-- Iteration 5 --
bool(true)
array(1) {
  [0]=>
  int(-20)
}
string(5) "array"
-- Iteration 6 --
bool(true)
array(1) {
  [0]=>
  float(2.54)
}
string(5) "array"
-- Iteration 7 --
bool(true)
array(1) {
  [0]=>
  float(-2.54)
}
string(5) "array"
-- Iteration 8 --
bool(true)
array(0) {
}
string(5) "array"
-- Iteration 9 --
bool(true)
array(1) {
  [0]=>
  bool(false)
}
string(5) "array"
-- Iteration 10 --
bool(true)
array(1) {
  [0]=>
  string(11) "some string"
}
string(5) "array"
-- Iteration 11 --
bool(true)
array(1) {
  [0]=>
  string(6) "string"
}
string(5) "array"
-- Iteration 12 --
bool(true)
array(1) {
  [0]=>
  string(11) "some string"
}
string(5) "array"
-- Iteration 13 --
bool(true)
array(1) {
  [0]=>
  string(6) "string"
}
string(5) "array"
-- Iteration 14 --
bool(true)
array(1) {
  [0]=>
  resource(5) of type (stream)
}
string(5) "array"
-- Iteration 15 --
bool(true)
array(1) {
  [0]=>
  resource(6) of type (stream)
}
string(5) "array"
-- Iteration 16 --
bool(true)
array(2) {
  ["x"]=>
  int(10)
  ["y"]=>
  int(20)
}
string(5) "array"

-- Setting type of data to object --
-- Iteration 1 --
bool(true)
object(stdClass)#2 (3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(6) "object"
-- Iteration 2 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  string(14) "another string"
}
string(6) "object"
-- Iteration 3 --
bool(true)
object(stdClass)#2 (3) {
  [0]=>
  int(2)
  [1]=>
  int(3)
  [2]=>
  int(4)
}
string(6) "object"
-- Iteration 4 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  int(1)
}
string(6) "object"
-- Iteration 5 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  int(-20)
}
string(6) "object"
-- Iteration 6 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  float(2.54)
}
string(6) "object"
-- Iteration 7 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  float(-2.54)
}
string(6) "object"
-- Iteration 8 --
bool(true)
object(stdClass)#2 (0) {
}
string(6) "object"
-- Iteration 9 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  bool(false)
}
string(6) "object"
-- Iteration 10 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  string(11) "some string"
}
string(6) "object"
-- Iteration 11 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  string(6) "string"
}
string(6) "object"
-- Iteration 12 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  string(11) "some string"
}
string(6) "object"
-- Iteration 13 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  string(6) "string"
}
string(6) "object"
-- Iteration 14 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  resource(5) of type (stream)
}
string(6) "object"
-- Iteration 15 --
bool(true)
object(stdClass)#2 (1) {
  ["scalar"]=>
  resource(6) of type (stream)
}
string(6) "object"
-- Iteration 16 --
bool(true)
object(point)#1 (2) {
  ["x"]=>
  int(10)
  ["y"]=>
  int(20)
}
string(6) "object"

-- Setting type of data to string --
-- Iteration 1 --
8: Array to string conversion
bool(true)
string(5) "Array"
string(6) "string"
-- Iteration 2 --
bool(true)
string(14) "another string"
string(6) "string"
-- Iteration 3 --
8: Array to string conversion
bool(true)
string(5) "Array"
string(6) "string"
-- Iteration 4 --
bool(true)
string(1) "1"
string(6) "string"
-- Iteration 5 --
bool(true)
string(3) "-20"
string(6) "string"
-- Iteration 6 --
bool(true)
string(4) "2.54"
string(6) "string"
-- Iteration 7 --
bool(true)
string(5) "-2.54"
string(6) "string"
-- Iteration 8 --
bool(true)
string(0) ""
string(6) "string"
-- Iteration 9 --
bool(true)
string(0) ""
string(6) "string"
-- Iteration 10 --
bool(true)
string(11) "some string"
string(6) "string"
-- Iteration 11 --
bool(true)
string(6) "string"
string(6) "string"
-- Iteration 12 --
bool(true)
string(11) "some string"
string(6) "string"
-- Iteration 13 --
bool(true)
string(6) "string"
string(6) "string"
-- Iteration 14 --
bool(true)
string(14) "Resource id #5"
string(6) "string"
-- Iteration 15 --
bool(true)
string(14) "Resource id #6"
string(6) "string"
-- Iteration 16 --
bool(true)
string(6) "Object"
string(6) "string"
Done
--UEXPECT--
**** Testing gettype() and settype() functions ****

*** Testing gettype(): basic operations ***
unicode(5) "array"
unicode(7) "unicode"
unicode(5) "array"
unicode(7) "integer"
unicode(7) "integer"
unicode(6) "double"
unicode(6) "double"
unicode(4) "NULL"
unicode(7) "boolean"
unicode(7) "unicode"
unicode(7) "unicode"
unicode(6) "string"
unicode(6) "string"
unicode(8) "resource"
unicode(8) "resource"
unicode(6) "object"

*** Testing settype(): basic operations ***

-- Setting type of data to null --
-- Iteration 1 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 2 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 3 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 4 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 5 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 6 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 7 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 8 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 9 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 10 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 11 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 12 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 13 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 14 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 15 --
bool(true)
NULL
unicode(4) "NULL"
-- Iteration 16 --
bool(true)
NULL
unicode(4) "NULL"

-- Setting type of data to integer --
-- Iteration 1 --
bool(true)
int(1)
unicode(7) "integer"
-- Iteration 2 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 3 --
bool(true)
int(1)
unicode(7) "integer"
-- Iteration 4 --
bool(true)
int(1)
unicode(7) "integer"
-- Iteration 5 --
bool(true)
int(-20)
unicode(7) "integer"
-- Iteration 6 --
bool(true)
int(2)
unicode(7) "integer"
-- Iteration 7 --
bool(true)
int(-2)
unicode(7) "integer"
-- Iteration 8 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 9 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 10 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 11 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 12 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 13 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 14 --
bool(true)
int(5)
unicode(7) "integer"
-- Iteration 15 --
bool(true)
int(6)
unicode(7) "integer"
-- Iteration 16 --
8: Object of class point could not be converted to int
bool(true)
int(1)
unicode(7) "integer"

-- Setting type of data to int --
-- Iteration 1 --
bool(true)
int(1)
unicode(7) "integer"
-- Iteration 2 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 3 --
bool(true)
int(1)
unicode(7) "integer"
-- Iteration 4 --
bool(true)
int(1)
unicode(7) "integer"
-- Iteration 5 --
bool(true)
int(-20)
unicode(7) "integer"
-- Iteration 6 --
bool(true)
int(2)
unicode(7) "integer"
-- Iteration 7 --
bool(true)
int(-2)
unicode(7) "integer"
-- Iteration 8 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 9 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 10 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 11 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 12 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 13 --
bool(true)
int(0)
unicode(7) "integer"
-- Iteration 14 --
bool(true)
int(5)
unicode(7) "integer"
-- Iteration 15 --
bool(true)
int(6)
unicode(7) "integer"
-- Iteration 16 --
8: Object of class point could not be converted to int
bool(true)
int(1)
unicode(7) "integer"

-- Setting type of data to float --
-- Iteration 1 --
bool(true)
float(1)
unicode(6) "double"
-- Iteration 2 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 3 --
bool(true)
float(1)
unicode(6) "double"
-- Iteration 4 --
bool(true)
float(1)
unicode(6) "double"
-- Iteration 5 --
bool(true)
float(-20)
unicode(6) "double"
-- Iteration 6 --
bool(true)
float(2.54)
unicode(6) "double"
-- Iteration 7 --
bool(true)
float(-2.54)
unicode(6) "double"
-- Iteration 8 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 9 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 10 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 11 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 12 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 13 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 14 --
bool(true)
float(5)
unicode(6) "double"
-- Iteration 15 --
bool(true)
float(6)
unicode(6) "double"
-- Iteration 16 --
8: Object of class point could not be converted to double
bool(true)
float(1)
unicode(6) "double"

-- Setting type of data to double --
-- Iteration 1 --
bool(true)
float(1)
unicode(6) "double"
-- Iteration 2 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 3 --
bool(true)
float(1)
unicode(6) "double"
-- Iteration 4 --
bool(true)
float(1)
unicode(6) "double"
-- Iteration 5 --
bool(true)
float(-20)
unicode(6) "double"
-- Iteration 6 --
bool(true)
float(2.54)
unicode(6) "double"
-- Iteration 7 --
bool(true)
float(-2.54)
unicode(6) "double"
-- Iteration 8 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 9 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 10 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 11 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 12 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 13 --
bool(true)
float(0)
unicode(6) "double"
-- Iteration 14 --
bool(true)
float(5)
unicode(6) "double"
-- Iteration 15 --
bool(true)
float(6)
unicode(6) "double"
-- Iteration 16 --
8: Object of class point could not be converted to double
bool(true)
float(1)
unicode(6) "double"

-- Setting type of data to boolean --
-- Iteration 1 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 2 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 3 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 4 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 5 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 6 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 7 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 8 --
bool(true)
bool(false)
unicode(7) "boolean"
-- Iteration 9 --
bool(true)
bool(false)
unicode(7) "boolean"
-- Iteration 10 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 11 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 12 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 13 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 14 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 15 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 16 --
bool(true)
bool(true)
unicode(7) "boolean"

-- Setting type of data to bool --
-- Iteration 1 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 2 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 3 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 4 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 5 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 6 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 7 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 8 --
bool(true)
bool(false)
unicode(7) "boolean"
-- Iteration 9 --
bool(true)
bool(false)
unicode(7) "boolean"
-- Iteration 10 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 11 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 12 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 13 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 14 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 15 --
bool(true)
bool(true)
unicode(7) "boolean"
-- Iteration 16 --
bool(true)
bool(true)
unicode(7) "boolean"

-- Setting type of data to resource --
-- Iteration 1 --
2: settype(): Cannot convert to resource type
bool(false)
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
unicode(5) "array"
-- Iteration 2 --
2: settype(): Cannot convert to resource type
bool(false)
unicode(14) "another string"
unicode(7) "unicode"
-- Iteration 3 --
2: settype(): Cannot convert to resource type
bool(false)
array(3) {
  [0]=>
  int(2)
  [1]=>
  int(3)
  [2]=>
  int(4)
}
unicode(5) "array"
-- Iteration 4 --
2: settype(): Cannot convert to resource type
bool(false)
int(1)
unicode(7) "integer"
-- Iteration 5 --
2: settype(): Cannot convert to resource type
bool(false)
int(-20)
unicode(7) "integer"
-- Iteration 6 --
2: settype(): Cannot convert to resource type
bool(false)
float(2.54)
unicode(6) "double"
-- Iteration 7 --
2: settype(): Cannot convert to resource type
bool(false)
float(-2.54)
unicode(6) "double"
-- Iteration 8 --
2: settype(): Cannot convert to resource type
bool(false)
NULL
unicode(4) "NULL"
-- Iteration 9 --
2: settype(): Cannot convert to resource type
bool(false)
bool(false)
unicode(7) "boolean"
-- Iteration 10 --
2: settype(): Cannot convert to resource type
bool(false)
unicode(11) "some string"
unicode(7) "unicode"
-- Iteration 11 --
2: settype(): Cannot convert to resource type
bool(false)
unicode(6) "string"
unicode(7) "unicode"
-- Iteration 12 --
2: settype(): Cannot convert to resource type
bool(false)
string(11) "some string"
unicode(6) "string"
-- Iteration 13 --
2: settype(): Cannot convert to resource type
bool(false)
string(6) "string"
unicode(6) "string"
-- Iteration 14 --
2: settype(): Cannot convert to resource type
bool(false)
resource(5) of type (stream)
unicode(8) "resource"
-- Iteration 15 --
2: settype(): Cannot convert to resource type
bool(false)
resource(6) of type (stream)
unicode(8) "resource"
-- Iteration 16 --
2: settype(): Cannot convert to resource type
bool(false)
object(point)#1 (2) {
  [u"x"]=>
  int(10)
  [u"y"]=>
  int(20)
}
unicode(6) "object"

-- Setting type of data to array --
-- Iteration 1 --
bool(true)
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
unicode(5) "array"
-- Iteration 2 --
bool(true)
array(1) {
  [0]=>
  unicode(14) "another string"
}
unicode(5) "array"
-- Iteration 3 --
bool(true)
array(3) {
  [0]=>
  int(2)
  [1]=>
  int(3)
  [2]=>
  int(4)
}
unicode(5) "array"
-- Iteration 4 --
bool(true)
array(1) {
  [0]=>
  int(1)
}
unicode(5) "array"
-- Iteration 5 --
bool(true)
array(1) {
  [0]=>
  int(-20)
}
unicode(5) "array"
-- Iteration 6 --
bool(true)
array(1) {
  [0]=>
  float(2.54)
}
unicode(5) "array"
-- Iteration 7 --
bool(true)
array(1) {
  [0]=>
  float(-2.54)
}
unicode(5) "array"
-- Iteration 8 --
bool(true)
array(0) {
}
unicode(5) "array"
-- Iteration 9 --
bool(true)
array(1) {
  [0]=>
  bool(false)
}
unicode(5) "array"
-- Iteration 10 --
bool(true)
array(1) {
  [0]=>
  unicode(11) "some string"
}
unicode(5) "array"
-- Iteration 11 --
bool(true)
array(1) {
  [0]=>
  unicode(6) "string"
}
unicode(5) "array"
-- Iteration 12 --
bool(true)
array(1) {
  [0]=>
  string(11) "some string"
}
unicode(5) "array"
-- Iteration 13 --
bool(true)
array(1) {
  [0]=>
  string(6) "string"
}
unicode(5) "array"
-- Iteration 14 --
bool(true)
array(1) {
  [0]=>
  resource(5) of type (stream)
}
unicode(5) "array"
-- Iteration 15 --
bool(true)
array(1) {
  [0]=>
  resource(6) of type (stream)
}
unicode(5) "array"
-- Iteration 16 --
bool(true)
array(2) {
  [u"x"]=>
  int(10)
  [u"y"]=>
  int(20)
}
unicode(5) "array"

-- Setting type of data to object --
-- Iteration 1 --
bool(true)
object(stdClass)#2 (3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
unicode(6) "object"
-- Iteration 2 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  unicode(14) "another string"
}
unicode(6) "object"
-- Iteration 3 --
bool(true)
object(stdClass)#2 (3) {
  [0]=>
  int(2)
  [1]=>
  int(3)
  [2]=>
  int(4)
}
unicode(6) "object"
-- Iteration 4 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  int(1)
}
unicode(6) "object"
-- Iteration 5 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  int(-20)
}
unicode(6) "object"
-- Iteration 6 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  float(2.54)
}
unicode(6) "object"
-- Iteration 7 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  float(-2.54)
}
unicode(6) "object"
-- Iteration 8 --
bool(true)
object(stdClass)#2 (0) {
}
unicode(6) "object"
-- Iteration 9 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  bool(false)
}
unicode(6) "object"
-- Iteration 10 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  unicode(11) "some string"
}
unicode(6) "object"
-- Iteration 11 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  unicode(6) "string"
}
unicode(6) "object"
-- Iteration 12 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  string(11) "some string"
}
unicode(6) "object"
-- Iteration 13 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  string(6) "string"
}
unicode(6) "object"
-- Iteration 14 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  resource(5) of type (stream)
}
unicode(6) "object"
-- Iteration 15 --
bool(true)
object(stdClass)#2 (1) {
  [u"scalar"]=>
  resource(6) of type (stream)
}
unicode(6) "object"
-- Iteration 16 --
bool(true)
object(point)#1 (2) {
  [u"x"]=>
  int(10)
  [u"y"]=>
  int(20)
}
unicode(6) "object"

-- Setting type of data to string --
-- Iteration 1 --
8: Array to string conversion
bool(true)
unicode(5) "Array"
unicode(7) "unicode"
-- Iteration 2 --
bool(true)
unicode(14) "another string"
unicode(7) "unicode"
-- Iteration 3 --
8: Array to string conversion
bool(true)
unicode(5) "Array"
unicode(7) "unicode"
-- Iteration 4 --
bool(true)
unicode(1) "1"
unicode(7) "unicode"
-- Iteration 5 --
bool(true)
unicode(3) "-20"
unicode(7) "unicode"
-- Iteration 6 --
bool(true)
unicode(4) "2.54"
unicode(7) "unicode"
-- Iteration 7 --
bool(true)
unicode(5) "-2.54"
unicode(7) "unicode"
-- Iteration 8 --
bool(true)
unicode(0) ""
unicode(7) "unicode"
-- Iteration 9 --
bool(true)
unicode(0) ""
unicode(7) "unicode"
-- Iteration 10 --
bool(true)
unicode(11) "some string"
unicode(7) "unicode"
-- Iteration 11 --
bool(true)
unicode(6) "string"
unicode(7) "unicode"
-- Iteration 12 --
bool(true)
unicode(11) "some string"
unicode(7) "unicode"
-- Iteration 13 --
bool(true)
unicode(6) "string"
unicode(7) "unicode"
-- Iteration 14 --
bool(true)
unicode(14) "Resource id #5"
unicode(7) "unicode"
-- Iteration 15 --
bool(true)
unicode(14) "Resource id #6"
unicode(7) "unicode"
-- Iteration 16 --
bool(true)
unicode(6) "Object"
unicode(7) "unicode"
Done
