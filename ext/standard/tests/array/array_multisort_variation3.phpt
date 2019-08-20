--TEST--
Test array_multisort() function : usage variation
--FILE--
<?php
/* Prototype  : bool array_multisort(array ar1 [, SORT_ASC|SORT_DESC [, SORT_REGULAR|SORT_NUMERIC|SORT_STRING|SORT_NATURAL|SORT_FLAG_CASE]] [, array ar2 [, SORT_ASC|SORT_DESC [, SORT_REGULAR|SORT_NUMERIC|SORT_STRING|SORT_NATURAL|SORT_FLAG_CASE]], ...])
 * Description: Sort multiple arrays at once similar to how ORDER BY clause works in SQL
 * Source code: ext/standard/array.c
 * Alias to functions:
 */

echo "*** Testing array_multisort() : usage variation ***\n";

// Define error handler
function test_error_handler($err_no, $err_msg, $filename, $linenum) {
	if (error_reporting() & $err_no) {
		// report non-silenced errors
		echo "Error: $err_no - $err_msg, $filename($linenum)\n";
	}
}
set_error_handler('test_error_handler');

// Initialise function arguments not being substituted (if any)
$ar1 = array(1, 2);

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

// loop through each element of the array for ar2
foreach($inputs as $key =>$value) {
    echo "\n--$key--\n";
    try {
        var_dump( array_multisort($ar1, SORT_REGULAR, $value) );
    } catch (Error $e) {
        echo $e->getMessage() . "\n";
    }
};

?>
===DONE===
--EXPECT--
*** Testing array_multisort() : usage variation ***

--int 0--
Argument #3 is expected to be an array or sorting flag that has not already been specified

--int 1--
Argument #3 is expected to be an array or sorting flag that has not already been specified

--int 12345--
Argument #3 is an unknown sort flag

--int -12345--
Argument #3 is an unknown sort flag

--float 10.5--
Argument #3 is expected to be an array or a sort flag

--float -10.5--
Argument #3 is expected to be an array or a sort flag

--float 12.3456789000e10--
Argument #3 is expected to be an array or a sort flag

--float -12.3456789000e10--
Argument #3 is expected to be an array or a sort flag

--float .5--
Argument #3 is expected to be an array or a sort flag

--uppercase NULL--
Argument #3 is expected to be an array or a sort flag

--lowercase null--
Argument #3 is expected to be an array or a sort flag

--lowercase true--
Argument #3 is expected to be an array or a sort flag

--lowercase false--
Argument #3 is expected to be an array or a sort flag

--uppercase TRUE--
Argument #3 is expected to be an array or a sort flag

--uppercase FALSE--
Argument #3 is expected to be an array or a sort flag

--empty string DQ--
Argument #3 is expected to be an array or a sort flag

--empty string SQ--
Argument #3 is expected to be an array or a sort flag

--string DQ--
Argument #3 is expected to be an array or a sort flag

--string SQ--
Argument #3 is expected to be an array or a sort flag

--mixed case string--
Argument #3 is expected to be an array or a sort flag

--heredoc--
Argument #3 is expected to be an array or a sort flag

--instance of classWithToString--
Argument #3 is expected to be an array or a sort flag

--instance of classWithoutToString--
Argument #3 is expected to be an array or a sort flag

--undefined var--
Argument #3 is expected to be an array or a sort flag

--unset var--
Argument #3 is expected to be an array or a sort flag
===DONE===
