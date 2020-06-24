--TEST--
Test get_parent_class() function : usage variations  - unexpected argument type.
--FILE--
<?php
spl_autoload_register(function ($className) {
    echo "In autoload($className)\n";
});

function test_error_handler($err_no, $err_msg, $filename, $linenum) {
    echo "Error: $err_no - $err_msg\n";
}
set_error_handler('test_error_handler');

echo "*** Testing get_parent_class() : usage variations ***\n";

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

      // object data
      new stdclass(),

      // undefined data
      $undefined_var,

      // unset data
      $unset_var,
);

// loop through each element of the array for object

foreach($values as $value) {
      echo "\nArg value " . (is_object($value) ? get_class($value) : $value) . " \n";
      var_dump( get_parent_class($value) );
};

echo "Done";
?>
--EXPECT--
*** Testing get_parent_class() : usage variations ***
Error: 2 - Undefined variable $undefined_var
Error: 2 - Undefined variable $unset_var

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
Error: 2 - Array to string conversion

Arg value Array 
bool(false)
Error: 2 - Array to string conversion

Arg value Array 
bool(false)
Error: 2 - Array to string conversion

Arg value Array 
bool(false)
Error: 2 - Array to string conversion

Arg value Array 
bool(false)
Error: 2 - Array to string conversion

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

Arg value stdClass 
bool(false)

Arg value  
bool(false)

Arg value  
bool(false)
Done
