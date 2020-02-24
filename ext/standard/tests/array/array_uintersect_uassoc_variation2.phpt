--TEST--
Test array_uintersect_uassoc() function : usage variation
--FILE--
<?php
/* Prototype  : array array_uintersect_uassoc(array arr1, array arr2 [, array ...], callback data_compare_func, callback key_compare_func)
 * Description: Returns the entries of arr1 that have values which are present in all the other arguments. Keys are used to do more restrictive check. Both data and keys are compared by using user-supplied callbacks.
 * Source code: ext/standard/array.c
 * Alias to functions:
 */

echo "*** Testing array_uintersect_uassoc() : usage variation ***\n";

// Initialise function arguments not being substituted (if any)
$arr1 = array(1, 2);

include('compare_function.inc');
$data_compare_func = 'compare_function';
$key_compare_func = 'compare_function';

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

//array of values to iterate over
$inputs = array(

      // int data
      'int 0' => 0,
      'int 1' => 1,
      'int 12345' => 12345,
      'int -12345' => -2345,

      // float data
      'float 10.5' => 10.5,
      'float -10.5' => -10.5,
      'float 12.3456789000e10' => 12.3456789000e10,
      'float -12.3456789000e10' => -12.3456789000e10,
      'float .5' => .5,

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
);

// loop through each element of the array for arr2

foreach($inputs as $key =>$value) {
    echo "\n--$key--\n";
    try {
        var_dump( array_uintersect_uassoc($arr1, $value, $data_compare_func, $key_compare_func) );
    } catch (TypeError $e) {
        echo $e->getMessage(), "\n";
    }
};

?>
--EXPECT--
*** Testing array_uintersect_uassoc() : usage variation ***

--int 0--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, int given

--int 1--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, int given

--int 12345--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, int given

--int -12345--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, int given

--float 10.5--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, float given

--float -10.5--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, float given

--float 12.3456789000e10--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, float given

--float -12.3456789000e10--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, float given

--float .5--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, float given

--uppercase NULL--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, null given

--lowercase null--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, null given

--lowercase true--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, bool given

--lowercase false--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, bool given

--uppercase TRUE--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, bool given

--uppercase FALSE--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, bool given

--empty string DQ--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, string given

--empty string SQ--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, string given

--string DQ--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, string given

--string SQ--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, string given

--mixed case string--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, string given

--heredoc--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, string given

--instance of classWithToString--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, object given

--instance of classWithoutToString--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, object given

--undefined var--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, null given

--unset var--
array_uintersect_uassoc(): Argument #2 ($arr2) must be of type array, null given
