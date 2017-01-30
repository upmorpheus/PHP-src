--TEST--
Test is_subclass_of() function : usage variations  - unexpected type for arg 1
--FILE--
<?php
/* Prototype  : proto bool is_subclass_of(object object, string class_name)
 * Description: Returns true if the object has this class as one of its parents 
 * Source code: Zend/zend_builtin_functions.c
 * Alias to functions: 
 */
// Note: basic use cases in Zend/tests/is_a.phpt
spl_autoload_register(function ($className) {
	echo "In autoload($className)\n";
});

function test_error_handler($err_no, $err_msg, $filename, $linenum, $vars) {
	echo "Error: $err_no - $err_msg, $filename($linenum)\n";
}
set_error_handler('test_error_handler');


echo "*** Testing is_subclass_of() : usage variations ***\n";

// Initialise function arguments not being substituted (if any)
$class_name = 'string_val';

//get an unset variable
$unset_var = 10;
unset ($unset_var);

//array of values to iterate over
$values = array(

      // int data
      0,
      1,
      12345,
      -2345,

      // float data
      10.5,
      -10.5,
      10.1234567e10,
      10.7654321E-10,
      .5,

      // array data
      array(),
      array(0),
      array(1),
      array(1, 2),
      array('color' => 'red', 'item' => 'pen'),

      // null data
      NULL,
      null,

      // boolean data
      true,
      false,
      TRUE,
      FALSE,

      // empty data
      "",
      '',

      // string data
      "string",
      'String',

      // undefined data
      $undefined_var,

      // unset data
      $unset_var,
);

// loop through each element of the array for object

foreach($values as $value) {
      echo "\nArg value $value \n";
      var_dump( is_subclass_of($value, $class_name) );
};

echo "Done";
?>
--EXPECTF--
*** Testing is_subclass_of() : usage variations ***
Error: 8 - Undefined variable: undefined_var, %s(69)
Error: 8 - Undefined variable: unset_var, %s(72)

Arg value 0 
bool(false)

Arg value 1 
bool(false)

Arg value 12345 
bool(false)

Arg value -2345 
bool(false)

Arg value 10.5 
bool(false)

Arg value -10.5 
bool(false)

Arg value 101234567000 
bool(false)

Arg value 1.07654321E-9 
bool(false)

Arg value 0.5 
bool(false)
Error: 8 - Array to string conversion, %sis_subclass_of_variation_001.php(%d)

Arg value Array 
bool(false)
Error: 8 - Array to string conversion, %sis_subclass_of_variation_001.php(%d)

Arg value Array 
bool(false)
Error: 8 - Array to string conversion, %sis_subclass_of_variation_001.php(%d)

Arg value Array 
bool(false)
Error: 8 - Array to string conversion, %sis_subclass_of_variation_001.php(%d)

Arg value Array 
bool(false)
Error: 8 - Array to string conversion, %sis_subclass_of_variation_001.php(%d)

Arg value Array 
bool(false)

Arg value  
bool(false)

Arg value  
bool(false)

Arg value 1 
bool(false)

Arg value  
bool(false)

Arg value 1 
bool(false)

Arg value  
bool(false)

Arg value  
bool(false)

Arg value  
bool(false)

Arg value string 
In autoload(string)
bool(false)

Arg value String 
In autoload(String)
bool(false)

Arg value  
bool(false)

Arg value  
bool(false)
Done
