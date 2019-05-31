--TEST--
Test array_intersect_key() function : usage variation - Passing unexpected values to second argument
--FILE--
<?php
/* Prototype  : array array_intersect_key(array arr1, array arr2 [, array ...])
 * Description: Returns the entries of arr1 that have keys which are present in all the other arguments.
 * Source code: ext/standard/array.c
 */

echo "*** Testing array_intersect_key() : usage variation ***\n";

// Initialise function arguments not being substituted (if any)
$array1 = array('blue'  => 1, 'red'  => 2, 'green'  => 3, 'purple' => 4);
$array3 = array('green' => 5, 'blue' => 6, 'yellow' => 7, 'cyan'   => 8);

//get an unset variable
$unset_var = 10;
unset ($unset_var);

//resource variable
$fp = fopen(__FILE__, "r");

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
      'int -12345' => -12345,

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

      // resource data
      'resource var' => $fp,
);

// loop through each element of the array for arr2

foreach($inputs as $key =>$value) {
    echo "\n--$key--\n";
    try {
        var_dump( array_intersect_key($array1, $value) );
    } catch (TypeError $e) {
        echo $e->getMessage(), "\n";
    }
    try {
        var_dump( array_intersect_key($array1, $value, $array3) );
    } catch (TypeError $e) {
        echo $e->getMessage(), "\n";
    }
}

fclose($fp);
?>
===DONE===
--EXPECT--
*** Testing array_intersect_key() : usage variation ***

--int 0--
Expected parameter 2 to be an array, int given
Expected parameter 2 to be an array, int given

--int 1--
Expected parameter 2 to be an array, int given
Expected parameter 2 to be an array, int given

--int 12345--
Expected parameter 2 to be an array, int given
Expected parameter 2 to be an array, int given

--int -12345--
Expected parameter 2 to be an array, int given
Expected parameter 2 to be an array, int given

--float 10.5--
Expected parameter 2 to be an array, float given
Expected parameter 2 to be an array, float given

--float -10.5--
Expected parameter 2 to be an array, float given
Expected parameter 2 to be an array, float given

--float 12.3456789000e10--
Expected parameter 2 to be an array, float given
Expected parameter 2 to be an array, float given

--float -12.3456789000e10--
Expected parameter 2 to be an array, float given
Expected parameter 2 to be an array, float given

--float .5--
Expected parameter 2 to be an array, float given
Expected parameter 2 to be an array, float given

--uppercase NULL--
Expected parameter 2 to be an array, null given
Expected parameter 2 to be an array, null given

--lowercase null--
Expected parameter 2 to be an array, null given
Expected parameter 2 to be an array, null given

--lowercase true--
Expected parameter 2 to be an array, bool given
Expected parameter 2 to be an array, bool given

--lowercase false--
Expected parameter 2 to be an array, bool given
Expected parameter 2 to be an array, bool given

--uppercase TRUE--
Expected parameter 2 to be an array, bool given
Expected parameter 2 to be an array, bool given

--uppercase FALSE--
Expected parameter 2 to be an array, bool given
Expected parameter 2 to be an array, bool given

--empty string DQ--
Expected parameter 2 to be an array, string given
Expected parameter 2 to be an array, string given

--empty string SQ--
Expected parameter 2 to be an array, string given
Expected parameter 2 to be an array, string given

--string DQ--
Expected parameter 2 to be an array, string given
Expected parameter 2 to be an array, string given

--string SQ--
Expected parameter 2 to be an array, string given
Expected parameter 2 to be an array, string given

--mixed case string--
Expected parameter 2 to be an array, string given
Expected parameter 2 to be an array, string given

--heredoc--
Expected parameter 2 to be an array, string given
Expected parameter 2 to be an array, string given

--instance of classWithToString--
Expected parameter 2 to be an array, object given
Expected parameter 2 to be an array, object given

--instance of classWithoutToString--
Expected parameter 2 to be an array, object given
Expected parameter 2 to be an array, object given

--undefined var--
Expected parameter 2 to be an array, null given
Expected parameter 2 to be an array, null given

--unset var--
Expected parameter 2 to be an array, null given
Expected parameter 2 to be an array, null given

--resource var--
Expected parameter 2 to be an array, resource given
Expected parameter 2 to be an array, resource given
===DONE===
