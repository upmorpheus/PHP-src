--TEST--
Test gettype() & settype() functions : usage variations
--FILE--
<?php
/* Prototype: string gettype ( mixed $var );
   Description: Returns the type of the PHP variable var

   Prototype: bool settype ( mixed &$var, string $type );
   Description: Set the type of variable var to type 
*/

/* Test usage variation of gettype() and settype() functions:
         settype() to array type.
   Set type of the data to "array" and verify using gettype
   Following are performed in the listed sequence:
     get the current type of the variable
     set the type of the variable to array type
     dump the variable to see its new data
     get the new type of the variable
*/

/* function to handle catchable errors */
function foo($errno, $errstr, $errfile, $errline) {
//	var_dump($errstr);
   // print error no and error string
   echo "$errno: $errstr\n";
}
//set the error handler, this is required as
// settype() would fail with catachable fatal error 
set_error_handler("foo"); 

$var1 = "another string";
$var2 = array(2,3,4);

// a variable which is unset
$unset_var = 10.5;
unset( $unset_var );

class point
{
  var $x;
  var $y;

  function point($x, $y) {
     $this->x = $x;
     $this->y = $y;
  }

  function __toString() {
     return "ObjectPoint";
  }
}

$var_values = array ( 
  /* nulls */
  null,  

  /* boolean */
  FALSE, 
  TRUE,
  true,
 
  /* strings */
  "\xFF",
  "\x66",
  "\0123",
  "",
  '',
  " ",
  ' ',
  /* numerics in the form of string */
  '10',
  "10",
  "10string",
  '10string',
  "1",  
  "-1",
  "1e2",
  " 1",
  "2974394749328742328432",
  "-1e-2",
  '1',
  '-1',
  '1e2',
  ' 1',
  '2974394749328742328432',
  '-1e-2',
  "0xff",
  '0x55',
  '0XA55',
  '0X123',
  "0123",
  '0123',
  "-0123",
  "+0123",
  '-0123',
  '+0123',
  "-0x80001", // invalid numerics as its prefix with sign or have decimal points
  "+0x80001",
  "-0x80001.5",
  "0x80001.5",
  "@$%#$%^$%^&^",

  /* arrays */
  array(),
  array(NULL),
  array(1,2,3,4),
  array(1 => "one", 2 => "two", "3" => "three", "four" => 4),
  array(1.5, 2.4, 6.5e6),

  /* integers */
  -2147483648, // max -ne int value
  2147483647,
  2147483649,
  1232147483649,
  0x55,
  0xF674593039, // a hex value > than max int
  -0X558F,
  0555,
  -0555,
  02224242434343152, // an octal value > than max int
  
  /* floats */
  1e5,
  -1e5,
  1E5, 
  -1E5,
  -1.5,
  .5,
  -.5,
  .5e6,
  -.5e6,
  -.5e-6,
  .5e+6,
  -.5e+6,
  .512E6,
  -.512E6,
  .512E-6,
  +.512E-6,
  .512E+6,
  -.512E+6,

  new point(NULL, NULL),
  new point(2.5, 40.5),
  new point(0, 0),

  /* undefined/unset vars */
  $unset_var,
  $undef_var,

  /* binary strings */
  b"10string",
  b'10string',
  b"+0123",
  b'-0123',
  b"0xff",
  b'0x55',
  b'1e2',
  b'2974394749328742328432',
  b"1e2",
  b'10string',
  b"10string"
);

/* test conversion to array type */
$type = "array";

echo "\n*** Testing gettype() & settype() functions : usage variations ***\n";
echo "\n-- Setting type of data to $type --\n";

$loop_count = 1;
foreach ($var_values as $var) {
  echo "-- Iteration $loop_count --\n"; $loop_count++;

  // get the current data type
  var_dump( gettype($var) );

  // convert it to null
  var_dump( settype($var, $type) );

  // dump the converted data
  var_dump( $var );

  // check the new type after conversion
  var_dump( gettype($var) );
}

echo "Done\n";
?>
--EXPECTF--	
8: Undefined variable: unset_var
8: Undefined variable: undef_var

*** Testing gettype() & settype() functions : usage variations ***

-- Setting type of data to array --
-- Iteration 1 --
string(4) "NULL"
bool(true)
array(0) {
}
string(5) "array"
-- Iteration 2 --
string(7) "boolean"
bool(true)
array(1) {
  [0]=>
  bool(false)
}
string(5) "array"
-- Iteration 3 --
string(7) "boolean"
bool(true)
array(1) {
  [0]=>
  bool(true)
}
string(5) "array"
-- Iteration 4 --
string(7) "boolean"
bool(true)
array(1) {
  [0]=>
  bool(true)
}
string(5) "array"
-- Iteration 5 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(1) "�"
}
string(5) "array"
-- Iteration 6 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(1) "f"
}
string(5) "array"
-- Iteration 7 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) "
3"
}
string(5) "array"
-- Iteration 8 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(0) ""
}
string(5) "array"
-- Iteration 9 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(0) ""
}
string(5) "array"
-- Iteration 10 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(1) " "
}
string(5) "array"
-- Iteration 11 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(1) " "
}
string(5) "array"
-- Iteration 12 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) "10"
}
string(5) "array"
-- Iteration 13 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) "10"
}
string(5) "array"
-- Iteration 14 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
string(5) "array"
-- Iteration 15 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
string(5) "array"
-- Iteration 16 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(1) "1"
}
string(5) "array"
-- Iteration 17 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) "-1"
}
string(5) "array"
-- Iteration 18 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(3) "1e2"
}
string(5) "array"
-- Iteration 19 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) " 1"
}
string(5) "array"
-- Iteration 20 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(22) "2974394749328742328432"
}
string(5) "array"
-- Iteration 21 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "-1e-2"
}
string(5) "array"
-- Iteration 22 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(1) "1"
}
string(5) "array"
-- Iteration 23 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) "-1"
}
string(5) "array"
-- Iteration 24 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(3) "1e2"
}
string(5) "array"
-- Iteration 25 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(2) " 1"
}
string(5) "array"
-- Iteration 26 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(22) "2974394749328742328432"
}
string(5) "array"
-- Iteration 27 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "-1e-2"
}
string(5) "array"
-- Iteration 28 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0xff"
}
string(5) "array"
-- Iteration 29 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0x55"
}
string(5) "array"
-- Iteration 30 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "0XA55"
}
string(5) "array"
-- Iteration 31 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "0X123"
}
string(5) "array"
-- Iteration 32 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0123"
}
string(5) "array"
-- Iteration 33 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0123"
}
string(5) "array"
-- Iteration 34 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "-0123"
}
string(5) "array"
-- Iteration 35 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "+0123"
}
string(5) "array"
-- Iteration 36 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "-0123"
}
string(5) "array"
-- Iteration 37 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "+0123"
}
string(5) "array"
-- Iteration 38 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "-0x80001"
}
string(5) "array"
-- Iteration 39 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "+0x80001"
}
string(5) "array"
-- Iteration 40 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(10) "-0x80001.5"
}
string(5) "array"
-- Iteration 41 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(9) "0x80001.5"
}
string(5) "array"
-- Iteration 42 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(12) "@$%#$%^$%^&^"
}
string(5) "array"
-- Iteration 43 --
string(5) "array"
bool(true)
array(0) {
}
string(5) "array"
-- Iteration 44 --
string(5) "array"
bool(true)
array(1) {
  [0]=>
  NULL
}
string(5) "array"
-- Iteration 45 --
string(5) "array"
bool(true)
array(4) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
}
string(5) "array"
-- Iteration 46 --
string(5) "array"
bool(true)
array(4) {
  [1]=>
  string(3) "one"
  [2]=>
  string(3) "two"
  [3]=>
  string(5) "three"
  ["four"]=>
  int(4)
}
string(5) "array"
-- Iteration 47 --
string(5) "array"
bool(true)
array(3) {
  [0]=>
  float(1.5)
  [1]=>
  float(2.4)
  [2]=>
  float(6500000)
}
string(5) "array"
-- Iteration 48 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-2147483648)
}
string(5) "array"
-- Iteration 49 --
string(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(2147483647)
}
string(5) "array"
-- Iteration 50 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(2147483649)
}
string(5) "array"
-- Iteration 51 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(1232147483649)
}
string(5) "array"
-- Iteration 52 --
string(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(85)
}
string(5) "array"
-- Iteration 53 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(1058513956921)
}
string(5) "array"
-- Iteration 54 --
string(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(-21903)
}
string(5) "array"
-- Iteration 55 --
string(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(365)
}
string(5) "array"
-- Iteration 56 --
string(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(-365)
}
string(5) "array"
-- Iteration 57 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(80561044571754)
}
string(5) "array"
-- Iteration 58 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(100000)
}
string(5) "array"
-- Iteration 59 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-100000)
}
string(5) "array"
-- Iteration 60 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(100000)
}
string(5) "array"
-- Iteration 61 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-100000)
}
string(5) "array"
-- Iteration 62 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-1.5)
}
string(5) "array"
-- Iteration 63 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(0.5)
}
string(5) "array"
-- Iteration 64 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-0.5)
}
string(5) "array"
-- Iteration 65 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(500000)
}
string(5) "array"
-- Iteration 66 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-500000)
}
string(5) "array"
-- Iteration 67 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-5.0E-7)
}
string(5) "array"
-- Iteration 68 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(500000)
}
string(5) "array"
-- Iteration 69 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-500000)
}
string(5) "array"
-- Iteration 70 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(512000)
}
string(5) "array"
-- Iteration 71 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-512000)
}
string(5) "array"
-- Iteration 72 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(5.12E-7)
}
string(5) "array"
-- Iteration 73 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(5.12E-7)
}
string(5) "array"
-- Iteration 74 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(512000)
}
string(5) "array"
-- Iteration 75 --
string(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-512000)
}
string(5) "array"
-- Iteration 76 --
string(6) "object"
bool(true)
array(2) {
  ["x"]=>
  NULL
  ["y"]=>
  NULL
}
string(5) "array"
-- Iteration 77 --
string(6) "object"
bool(true)
array(2) {
  ["x"]=>
  float(2.5)
  ["y"]=>
  float(40.5)
}
string(5) "array"
-- Iteration 78 --
string(6) "object"
bool(true)
array(2) {
  ["x"]=>
  int(0)
  ["y"]=>
  int(0)
}
string(5) "array"
-- Iteration 79 --
string(4) "NULL"
bool(true)
array(0) {
}
string(5) "array"
-- Iteration 80 --
string(4) "NULL"
bool(true)
array(0) {
}
string(5) "array"
-- Iteration 81 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
string(5) "array"
-- Iteration 82 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
string(5) "array"
-- Iteration 83 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "+0123"
}
string(5) "array"
-- Iteration 84 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "-0123"
}
string(5) "array"
-- Iteration 85 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0xff"
}
string(5) "array"
-- Iteration 86 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0x55"
}
string(5) "array"
-- Iteration 87 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(3) "1e2"
}
string(5) "array"
-- Iteration 88 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(22) "2974394749328742328432"
}
string(5) "array"
-- Iteration 89 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(3) "1e2"
}
string(5) "array"
-- Iteration 90 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
string(5) "array"
-- Iteration 91 --
string(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
string(5) "array"
Done
--UEXPECTF--
8: Undefined variable: unset_var
8: Undefined variable: undef_var

*** Testing gettype() & settype() functions : usage variations ***

-- Setting type of data to array --
-- Iteration 1 --
unicode(4) "NULL"
bool(true)
array(0) {
}
unicode(5) "array"
-- Iteration 2 --
unicode(7) "boolean"
bool(true)
array(1) {
  [0]=>
  bool(false)
}
unicode(5) "array"
-- Iteration 3 --
unicode(7) "boolean"
bool(true)
array(1) {
  [0]=>
  bool(true)
}
unicode(5) "array"
-- Iteration 4 --
unicode(7) "boolean"
bool(true)
array(1) {
  [0]=>
  bool(true)
}
unicode(5) "array"
-- Iteration 5 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(1) "ÿ"
}
unicode(5) "array"
-- Iteration 6 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(1) "f"
}
unicode(5) "array"
-- Iteration 7 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) "
3"
}
unicode(5) "array"
-- Iteration 8 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(0) ""
}
unicode(5) "array"
-- Iteration 9 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(0) ""
}
unicode(5) "array"
-- Iteration 10 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(1) " "
}
unicode(5) "array"
-- Iteration 11 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(1) " "
}
unicode(5) "array"
-- Iteration 12 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) "10"
}
unicode(5) "array"
-- Iteration 13 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) "10"
}
unicode(5) "array"
-- Iteration 14 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(8) "10string"
}
unicode(5) "array"
-- Iteration 15 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(8) "10string"
}
unicode(5) "array"
-- Iteration 16 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(1) "1"
}
unicode(5) "array"
-- Iteration 17 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) "-1"
}
unicode(5) "array"
-- Iteration 18 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(3) "1e2"
}
unicode(5) "array"
-- Iteration 19 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) " 1"
}
unicode(5) "array"
-- Iteration 20 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(22) "2974394749328742328432"
}
unicode(5) "array"
-- Iteration 21 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "-1e-2"
}
unicode(5) "array"
-- Iteration 22 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(1) "1"
}
unicode(5) "array"
-- Iteration 23 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) "-1"
}
unicode(5) "array"
-- Iteration 24 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(3) "1e2"
}
unicode(5) "array"
-- Iteration 25 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(2) " 1"
}
unicode(5) "array"
-- Iteration 26 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(22) "2974394749328742328432"
}
unicode(5) "array"
-- Iteration 27 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "-1e-2"
}
unicode(5) "array"
-- Iteration 28 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(4) "0xff"
}
unicode(5) "array"
-- Iteration 29 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(4) "0x55"
}
unicode(5) "array"
-- Iteration 30 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "0XA55"
}
unicode(5) "array"
-- Iteration 31 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "0X123"
}
unicode(5) "array"
-- Iteration 32 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(4) "0123"
}
unicode(5) "array"
-- Iteration 33 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(4) "0123"
}
unicode(5) "array"
-- Iteration 34 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "-0123"
}
unicode(5) "array"
-- Iteration 35 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "+0123"
}
unicode(5) "array"
-- Iteration 36 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "-0123"
}
unicode(5) "array"
-- Iteration 37 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(5) "+0123"
}
unicode(5) "array"
-- Iteration 38 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(8) "-0x80001"
}
unicode(5) "array"
-- Iteration 39 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(8) "+0x80001"
}
unicode(5) "array"
-- Iteration 40 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(10) "-0x80001.5"
}
unicode(5) "array"
-- Iteration 41 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(9) "0x80001.5"
}
unicode(5) "array"
-- Iteration 42 --
unicode(7) "unicode"
bool(true)
array(1) {
  [0]=>
  unicode(12) "@$%#$%^$%^&^"
}
unicode(5) "array"
-- Iteration 43 --
unicode(5) "array"
bool(true)
array(0) {
}
unicode(5) "array"
-- Iteration 44 --
unicode(5) "array"
bool(true)
array(1) {
  [0]=>
  NULL
}
unicode(5) "array"
-- Iteration 45 --
unicode(5) "array"
bool(true)
array(4) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
}
unicode(5) "array"
-- Iteration 46 --
unicode(5) "array"
bool(true)
array(4) {
  [1]=>
  unicode(3) "one"
  [2]=>
  unicode(3) "two"
  [3]=>
  unicode(5) "three"
  [u"four"]=>
  int(4)
}
unicode(5) "array"
-- Iteration 47 --
unicode(5) "array"
bool(true)
array(3) {
  [0]=>
  float(1.5)
  [1]=>
  float(2.4)
  [2]=>
  float(6500000)
}
unicode(5) "array"
-- Iteration 48 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-2147483648)
}
unicode(5) "array"
-- Iteration 49 --
unicode(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(2147483647)
}
unicode(5) "array"
-- Iteration 50 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(2147483649)
}
unicode(5) "array"
-- Iteration 51 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(1232147483649)
}
unicode(5) "array"
-- Iteration 52 --
unicode(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(85)
}
unicode(5) "array"
-- Iteration 53 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(1058513956921)
}
unicode(5) "array"
-- Iteration 54 --
unicode(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(-21903)
}
unicode(5) "array"
-- Iteration 55 --
unicode(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(365)
}
unicode(5) "array"
-- Iteration 56 --
unicode(7) "integer"
bool(true)
array(1) {
  [0]=>
  int(-365)
}
unicode(5) "array"
-- Iteration 57 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(80561044571754)
}
unicode(5) "array"
-- Iteration 58 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(100000)
}
unicode(5) "array"
-- Iteration 59 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-100000)
}
unicode(5) "array"
-- Iteration 60 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(100000)
}
unicode(5) "array"
-- Iteration 61 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-100000)
}
unicode(5) "array"
-- Iteration 62 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-1.5)
}
unicode(5) "array"
-- Iteration 63 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(0.5)
}
unicode(5) "array"
-- Iteration 64 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-0.5)
}
unicode(5) "array"
-- Iteration 65 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(500000)
}
unicode(5) "array"
-- Iteration 66 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-500000)
}
unicode(5) "array"
-- Iteration 67 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-5.0E-7)
}
unicode(5) "array"
-- Iteration 68 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(500000)
}
unicode(5) "array"
-- Iteration 69 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-500000)
}
unicode(5) "array"
-- Iteration 70 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(512000)
}
unicode(5) "array"
-- Iteration 71 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-512000)
}
unicode(5) "array"
-- Iteration 72 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(5.12E-7)
}
unicode(5) "array"
-- Iteration 73 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(5.12E-7)
}
unicode(5) "array"
-- Iteration 74 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(512000)
}
unicode(5) "array"
-- Iteration 75 --
unicode(6) "double"
bool(true)
array(1) {
  [0]=>
  float(-512000)
}
unicode(5) "array"
-- Iteration 76 --
unicode(6) "object"
bool(true)
array(2) {
  [u"x"]=>
  NULL
  [u"y"]=>
  NULL
}
unicode(5) "array"
-- Iteration 77 --
unicode(6) "object"
bool(true)
array(2) {
  [u"x"]=>
  float(2.5)
  [u"y"]=>
  float(40.5)
}
unicode(5) "array"
-- Iteration 78 --
unicode(6) "object"
bool(true)
array(2) {
  [u"x"]=>
  int(0)
  [u"y"]=>
  int(0)
}
unicode(5) "array"
-- Iteration 79 --
unicode(4) "NULL"
bool(true)
array(0) {
}
unicode(5) "array"
-- Iteration 80 --
unicode(4) "NULL"
bool(true)
array(0) {
}
unicode(5) "array"
-- Iteration 81 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
unicode(5) "array"
-- Iteration 82 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
unicode(5) "array"
-- Iteration 83 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "+0123"
}
unicode(5) "array"
-- Iteration 84 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(5) "-0123"
}
unicode(5) "array"
-- Iteration 85 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0xff"
}
unicode(5) "array"
-- Iteration 86 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(4) "0x55"
}
unicode(5) "array"
-- Iteration 87 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(3) "1e2"
}
unicode(5) "array"
-- Iteration 88 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(22) "2974394749328742328432"
}
unicode(5) "array"
-- Iteration 89 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(3) "1e2"
}
unicode(5) "array"
-- Iteration 90 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
unicode(5) "array"
-- Iteration 91 --
unicode(6) "string"
bool(true)
array(1) {
  [0]=>
  string(8) "10string"
}
unicode(5) "array"
Done
