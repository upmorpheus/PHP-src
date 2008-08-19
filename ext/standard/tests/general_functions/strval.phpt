--TEST--
Test strval() function
--INI--
precision=14
--FILE--
<?php
/* Prototype: string strval ( mixed $var );
 * Description: Returns the string value of var
 */

echo "*** Testing str_val() with scalar values***\n";
$heredoc_string = <<<EOD
This is a multiline heredoc
string. Numeric = 1232455.
EOD;
/* heredoc string with only numeric values */
$heredoc_numeric_string = <<<EOD
12345
2345
EOD;
/* null heredoc string */
$heredoc_empty_string = <<<EOD
EOD;
/* heredoc string with NULL */ 
$heredoc_NULL_string = <<<EOD
NULL
EOD;

// different valid  scalar vlaues 
$scalars = array(
  /* integers */
  0,
  1,
  -1,
  -2147483648, // max negative integer value
  -2147483647, 
  2147483647,  // max positive integer value
  2147483640,
  0x123B,      // integer as hexadecimal
  0x12ab,
  0Xfff,
  0XFA,
 
  /* floats */ 
  -0x80000000, // max negative integer as hexadecimal
  0x7fffffff,  // max postive integer as hexadecimal
  0x7FFFFFFF,  // max postive integer as hexadecimal
  0123,        // integer as octal
  01912,       // should be quivalent to octal 1
  -020000000000, // max negative integer as octal
  017777777777,  // max positive integer as octal
  -2147483649, // float value
  2147483648,  // float value
  -0x80000001, // float value, beyond max negative int
  0x800000001, // float value, beyond max positive int
  020000000001, // float value, beyond max positive int
  -020000000001, // float value, beyond max negative int
  0.0,
  -0.1,
  10.0000000000000000005,
  10.5e+5,
  1e-5,
  .5e+7,
  .6e-19,
  .05E+44,
  .0034E-30,

  /* booleans */
  true,  
  TRUE,
  FALSE,
  false,  

  /* strings */  
  "",
  '',
  " ",
  ' ',
  '0',
  "0",
  "testing",
  "0x564",
  "0123",
  "new\n",
  'new\n',
  "@#$$%^&&*()",
  "        ",
  "null",
  'null',
  'true',
  "true",
  /*"\0123",
  "\0x12FF",*/
  $heredoc_string, 
  $heredoc_numeric_string,
  $heredoc_empty_string
);
/* loop to check that strval() recognizes different 
   scalar values and retuns the string conversion of same */
$loop_counter = 1;
foreach ($scalars as $scalar ) {
   echo "-- Iteration $loop_counter --\n"; $loop_counter++;
   var_dump( strval($scalar) );
}

echo "\n*** Testing strval() with non_scalar values ***\n";
// get a resource type variable
$fp = fopen(__FILE__, "r");
$dfp = opendir( dirname(__FILE__) );

// unset variable
$unset_var = 10;
unset ($unset_var);

// non_scalar values, objects, arrays, resources and boolean 
class foo
{
  function __toString() {
    return "Object";
  }
}

$not_scalars = array (
  new foo, //object
  $fp,  // resource
  $dfp,
  array(),  // arrays
  array(NULL),
  array(1,2,3,4),
  array("string"),
  NULL,  // nulls
  null,
  @$unset_var,  // unset variable
  @$undefined_var
);
/* loop through the $not_scalars to see working of 
   strval() on objects, arrays, boolean and others */
$loop_counter = 1;
foreach ($not_scalars as $value ) {
   echo "-- Iteration $loop_counter --\n"; $loop_counter++;
   var_dump( strval($value) );
}

echo "\n*** Testing error conditions ***\n";
//Zero argument
var_dump( strval() );

//arguments more than expected 
var_dump( strval( $scalars[0], $scalars[1]) );
 
echo "Done\n";
?>

--CLEAN--
// close the resources used
fclose($fp);
closedir($dfp);

--EXPECTF--
*** Testing str_val() with scalar values***
-- Iteration 1 --
unicode(1) "0"
-- Iteration 2 --
unicode(1) "1"
-- Iteration 3 --
unicode(2) "-1"
-- Iteration 4 --
unicode(11) "-2147483648"
-- Iteration 5 --
unicode(11) "-2147483647"
-- Iteration 6 --
unicode(10) "2147483647"
-- Iteration 7 --
unicode(10) "2147483640"
-- Iteration 8 --
unicode(4) "4667"
-- Iteration 9 --
unicode(4) "4779"
-- Iteration 10 --
unicode(4) "4095"
-- Iteration 11 --
unicode(3) "250"
-- Iteration 12 --
unicode(11) "-2147483648"
-- Iteration 13 --
unicode(10) "2147483647"
-- Iteration 14 --
unicode(10) "2147483647"
-- Iteration 15 --
unicode(2) "83"
-- Iteration 16 --
unicode(1) "1"
-- Iteration 17 --
unicode(11) "-2147483648"
-- Iteration 18 --
unicode(10) "2147483647"
-- Iteration 19 --
unicode(11) "-2147483649"
-- Iteration 20 --
unicode(10) "2147483648"
-- Iteration 21 --
unicode(11) "-2147483649"
-- Iteration 22 --
unicode(11) "34359738369"
-- Iteration 23 --
unicode(10) "2147483649"
-- Iteration 24 --
unicode(11) "-2147483649"
-- Iteration 25 --
unicode(1) "0"
-- Iteration 26 --
unicode(4) "-0.1"
-- Iteration 27 --
unicode(2) "10"
-- Iteration 28 --
unicode(7) "1050000"
-- Iteration 29 --
unicode(6) "1.0E-5"
-- Iteration 30 --
unicode(7) "5000000"
-- Iteration 31 --
unicode(7) "6.0E-20"
-- Iteration 32 --
unicode(7) "5.0E+42"
-- Iteration 33 --
unicode(7) "3.4E-33"
-- Iteration 34 --
unicode(1) "1"
-- Iteration 35 --
unicode(1) "1"
-- Iteration 36 --
unicode(0) ""
-- Iteration 37 --
unicode(0) ""
-- Iteration 38 --
unicode(0) ""
-- Iteration 39 --
unicode(0) ""
-- Iteration 40 --
unicode(1) " "
-- Iteration 41 --
unicode(1) " "
-- Iteration 42 --
unicode(1) "0"
-- Iteration 43 --
unicode(1) "0"
-- Iteration 44 --
unicode(7) "testing"
-- Iteration 45 --
unicode(5) "0x564"
-- Iteration 46 --
unicode(4) "0123"
-- Iteration 47 --
unicode(4) "new
"
-- Iteration 48 --
unicode(5) "new\n"
-- Iteration 49 --
unicode(11) "@#$$%^&&*()"
-- Iteration 50 --
unicode(8) "        "
-- Iteration 51 --
unicode(4) "null"
-- Iteration 52 --
unicode(4) "null"
-- Iteration 53 --
unicode(4) "true"
-- Iteration 54 --
unicode(4) "true"
-- Iteration 55 --
unicode(54) "This is a multiline heredoc
string. Numeric = 1232455."
-- Iteration 56 --
unicode(10) "12345
2345"
-- Iteration 57 --
unicode(0) ""

*** Testing strval() with non_scalar values ***
-- Iteration 1 --
unicode(6) "Object"
-- Iteration 2 --
unicode(14) "Resource id #%d"
-- Iteration 3 --
unicode(14) "Resource id #%d"
-- Iteration 4 --

Notice: Array to string conversion in %s on line %d
unicode(5) "Array"
-- Iteration 5 --

Notice: Array to string conversion in %s on line %d
unicode(5) "Array"
-- Iteration 6 --

Notice: Array to string conversion in %s on line %d
unicode(5) "Array"
-- Iteration 7 --

Notice: Array to string conversion in %s on line %d
unicode(5) "Array"
-- Iteration 8 --
unicode(0) ""
-- Iteration 9 --
unicode(0) ""
-- Iteration 10 --
unicode(0) ""
-- Iteration 11 --
unicode(0) ""

*** Testing error conditions ***

Warning: strval() expects exactly 1 parameter, 0 given in %s on line %d
NULL

Warning: strval() expects exactly 1 parameter, 2 given in %s on line %d
NULL
Done
