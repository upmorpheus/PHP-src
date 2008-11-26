--TEST--
Test dirname() function : usage variation 
--CREDITS--
Dave Kelsey <d_kelsey@uk.ibm.com>
--FILE--
<?php
/* Prototype  : string dirname(string path)
 * Description: Returns the directory name component of the path 
 * Source code: ext/standard/string.c
 * Alias to functions: 
 */

echo "*** Testing dirname() : usage variation ***\n";

// Define error handler
function test_error_handler($err_no, $err_msg, $filename, $linenum, $vars) {
	if (error_reporting() != 0) {
		// report non-silenced errors
		echo "Error: $err_no - $err_msg, $filename($linenum)\n";
	}
}
set_error_handler('test_error_handler');

// Initialise function arguments not being substituted (if any)

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

      // object data
      'instance of classWithToString' => new classWithToString(),
      'instance of classWithoutToString' => new classWithoutToString(),

      // undefined data
      'undefined var' => @$undefined_var,

      // unset data
      'unset var' => @$unset_var,
);

// loop through each element of the array for path

foreach($inputs as $key =>$value) {
      echo "\n--$key--\n";
      var_dump( dirname($value) );
};

?>
===DONE===
--EXPECTF--
*** Testing dirname() : usage variation ***

--int 0--
unicode(1) "."

--int 1--
unicode(1) "."

--int 12345--
unicode(1) "."

--int -12345--
unicode(1) "."

--float 10.5--
unicode(1) "."

--float -10.5--
unicode(1) "."

--float 12.3456789000e10--
unicode(1) "."

--float -12.3456789000e10--
unicode(1) "."

--float .5--
unicode(1) "."

--empty array--
Error: 2 - dirname() expects parameter 1 to be string (Unicode or binary), array given, %s(%d)
NULL

--int indexed array--
Error: 2 - dirname() expects parameter 1 to be string (Unicode or binary), array given, %s(%d)
NULL

--associative array--
Error: 2 - dirname() expects parameter 1 to be string (Unicode or binary), array given, %s(%d)
NULL

--nested arrays--
Error: 2 - dirname() expects parameter 1 to be string (Unicode or binary), array given, %s(%d)
NULL

--uppercase NULL--
unicode(0) ""

--lowercase null--
unicode(0) ""

--lowercase true--
unicode(1) "."

--lowercase false--
unicode(0) ""

--uppercase TRUE--
unicode(1) "."

--uppercase FALSE--
unicode(0) ""

--empty string DQ--
unicode(0) ""

--empty string SQ--
unicode(0) ""

--instance of classWithToString--
unicode(1) "."

--instance of classWithoutToString--
Error: 2 - dirname() expects parameter 1 to be string (Unicode or binary), object given, %s(%d)
NULL

--undefined var--
unicode(0) ""

--unset var--
unicode(0) ""
===DONE===

