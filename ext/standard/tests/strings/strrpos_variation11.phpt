--TEST--
Test strrpos() function : usage variations - unexpected inputs for 'haystack' and 'needle' arguments
--FILE--
<?php
/* Prototype  : int strrpos ( string $haystack, string $needle [, int $offset] );
 * Description: Find position of last occurrence of 'needle' in 'haystack'.
 * Source code: ext/standard/string.c
*/

/* Test strrpos() function with unexpected inputs for 'haystack' and 'needle' arguments */

echo "*** Testing strrpos() function with unexpected values for haystack and needle ***\n";

// get an unset variable
$unset_var = 'string_val';
unset($unset_var);

// defining a class
class sample  {
  public function __toString() {
    return "object";
  } 
}

//getting the resource
$file_handle = fopen(__FILE__, "r"); 

// array with different values
$values =  array (

  // integer values
  0,
  1,
  12345,
  -2345,

  // float values
  10.5,
  -10.5,
  10.5e10,
  10.6E-10,
  .5,

  // array values
  array(),
  array(0),
  array(1),
  array(1, 2),
  array('color' => 'red', 'item' => 'pen'),

  // boolean values
  true,
  false,
  TRUE,
  FALSE,

  // objects
  new sample(),

  // empty string
  "",
  '',

  // null values
  NULL,
  null,

  // resource
  $file_handle,

  // undefined variable
  @$undefined_var,

  // unset variable
  @$unset_var
);


// loop through each element of the array and check the working of strrpos()
$counter = 1;
for($index = 0; $index < count($values); $index ++) {
  echo "-- Iteration $counter --\n";
  $haystack = $values[$index];
  var_dump( strrpos($values[$index], $values[$index]) );
  var_dump( strrpos($values[$index], $values[$index], 1) );
  $counter ++;
}

echo "*** Done ***";
?>
--EXPECTF--
*** Testing strrpos() function with unexpected values for haystack and needle ***
-- Iteration 1 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 2 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 3 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 4 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 5 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 6 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 7 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 8 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 9 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 10 --

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)
-- Iteration 11 --

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)
-- Iteration 12 --

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)
-- Iteration 13 --

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)
-- Iteration 14 --

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)

Warning: strrpos() expects parameter 1 to be string, array given in %s on line %d
bool(false)
-- Iteration 15 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 16 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 17 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 18 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 19 --

Notice: Object of class sample could not be converted to int in %s on line %d

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Notice: Object of class sample could not be converted to int in %s on line %d

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 20 --
bool(false)
bool(false)
-- Iteration 21 --
bool(false)
bool(false)
-- Iteration 22 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 23 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 24 --

Warning: strrpos() expects parameter 1 to be string, resource given in %s on line %d
bool(false)

Warning: strrpos() expects parameter 1 to be string, resource given in %s on line %d
bool(false)
-- Iteration 25 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
-- Iteration 26 --

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)

Deprecated: strrpos(): Non-string needles will be interpreted as strings in %s on line %d
bool(false)
*** Done ***