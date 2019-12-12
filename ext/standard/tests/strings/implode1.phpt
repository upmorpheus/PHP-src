--TEST--
Test implode() function
--FILE--
<?php
/* Prototype: string implode ( string $glue, array $pieces );
   Description: Returns a string containing a string representation of all the
                array elements in the same order, with the glue string between each element.
*/
echo "*** Testing implode() for basic operations ***\n";
$arrays = array (
  array(1,2),
  array(1.1,2.2),
  array(array(2),array(1)),
  array(false,true),
  array(),
  array("a","aaaa","b","bbbb","c","ccccccccccccccccccccc")
);
/* loop to output string with ', ' as $glue, using implode() */
foreach ($arrays as $array) {
  var_dump( implode(', ', $array) );
  var_dump($array);
}

echo "\n*** Testing implode() with variations of glue ***\n";
/* checking possible variations */
$pieces = array (
  2,
  0,
  -639,
  true,
  "PHP",
  false,
  NULL,
  "",
  " ",
  "string\x00with\x00...\0"
);
$glues = array (
  "TRUE",
  true,
  false,
  array("key1", "key2"),
  "",
  " ",
  "string\x00between",
  NULL,
  -0,
  '\0'
);
/* loop through to display a string containing all the array $pieces in the same order,
   with the $glue string between each element  */
$counter = 1;
foreach($glues as $glue) {
  echo "-- Iteration $counter --\n";
  try {
       var_dump(implode($glue, $pieces));
  } catch (TypeError $exception) {
      echo $exception->getMessage() . "\n";
  }
  $counter++;
}

/* empty string */
echo "\n*** Testing implode() on empty string ***\n";
try {
    implode("");
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* checking sub-arrays */
echo "\n*** Testing implode() on sub-arrays ***\n";
$sub_array = array(array(1,2,3,4), array(1 => "one", 2 => "two"), "PHP", 50);
var_dump(implode("TEST", $sub_array));
try {
   var_dump(implode(array(1, 2, 3, 4), $sub_array));
} catch (TypeError $exception) {
  echo $exception->getMessage() . "\n";
}
try {
   var_dump( implode(2, $sub_array) );
} catch (TypeError $exception) {
  echo $exception->getMessage() . "\n";
}

echo "\n*** Testing implode() on objects ***\n";
/* checking on objects */
class foo
{
  function __toString() {
    return "Object";
  }
}

$obj = new foo(); //creating new object
$arr = array();
$arr[0] = &$obj;
$arr[1] = &$obj;
var_dump( implode(",", $arr) );
var_dump($arr);

/* Checking on resource types */
echo "\n*** Testing end() on resource type ***\n";
/* file type resource */
$file_handle = fopen(__FILE__, "r");

/* directory type resource */
$dir_handle = opendir( __DIR__ );

/* store resources in array for comparison */
$resources = array($file_handle, $dir_handle);

var_dump( implode("::", $resources) );

echo "\n*** Testing error conditions ***\n";

/* only glue */
try {
    var_dump( implode("glue") );
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* int as pieces */
try {
    var_dump( implode("glue",1234) );
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* NULL as pieces */
try {
    var_dump( implode("glue", NULL) );
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* pieces as NULL array */
try {
    var_dump( implode(",", array(NULL)) );
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* integer as glue */
try {
    var_dump( implode(12, "pieces") );
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* NULL as glue */
try {
    var_dump( implode(NULL, "abcd") );
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

/* closing resource handles */
fclose($file_handle);
closedir($dir_handle);

echo "Done\n";
?>
--EXPECTF--
*** Testing implode() for basic operations ***
string(4) "1, 2"
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}
string(8) "1.1, 2.2"
array(2) {
  [0]=>
  float(1.1)
  [1]=>
  float(2.2)
}

Warning: Array to string conversion in %s on line %d

Warning: Array to string conversion in %s on line %d
string(12) "Array, Array"
array(2) {
  [0]=>
  array(1) {
    [0]=>
    int(2)
  }
  [1]=>
  array(1) {
    [0]=>
    int(1)
  }
}
string(3) ", 1"
array(2) {
  [0]=>
  bool(false)
  [1]=>
  bool(true)
}
string(0) ""
array(0) {
}
string(42) "a, aaaa, b, bbbb, c, ccccccccccccccccccccc"
array(6) {
  [0]=>
  string(1) "a"
  [1]=>
  string(4) "aaaa"
  [2]=>
  string(1) "b"
  [3]=>
  string(4) "bbbb"
  [4]=>
  string(1) "c"
  [5]=>
  string(21) "ccccccccccccccccccccc"
}

*** Testing implode() with variations of glue ***
-- Iteration 1 --
string(63) "2TRUE0TRUE-639TRUE1TRUEPHPTRUETRUETRUETRUE TRUEstring with ... "
-- Iteration 2 --
string(36) "2101-639111PHP1111 1string with ... "
-- Iteration 3 --
string(27) "20-6391PHP string with ... "
-- Iteration 4 --
The first argument must be a string
-- Iteration 5 --
string(27) "20-6391PHP string with ... "
-- Iteration 6 --
string(36) "2 0 -639 1 PHP      string with ... "
-- Iteration 7 --
string(153) "2string between0string between-639string between1string betweenPHPstring betweenstring betweenstring betweenstring between string betweenstring with ... "
-- Iteration 8 --
string(27) "20-6391PHP string with ... "
-- Iteration 9 --
string(36) "2000-639010PHP0000 0string with ... "
-- Iteration 10 --
string(45) "2\00\0-639\01\0PHP\0\0\0\0 \0string with ... "

*** Testing implode() on empty string ***
Argument must be an array

*** Testing implode() on sub-arrays ***

Warning: Array to string conversion in %s on line %d

Warning: Array to string conversion in %s on line %d
string(27) "ArrayTESTArrayTESTPHPTEST50"
The first argument must be a string

Warning: Array to string conversion in %s

Warning: Array to string conversion in %s
string(18) "Array2Array2PHP250"

*** Testing implode() on objects ***
string(13) "Object,Object"
array(2) {
  [0]=>
  &object(foo)#%d (0) {
  }
  [1]=>
  &object(foo)#%d (0) {
  }
}

*** Testing end() on resource type ***
string(%d) "Resource id #%d::Resource id #%d"

*** Testing error conditions ***
Argument must be an array
implode() expects parameter 2 to be array, int given
implode() expects parameter 2 to be array, null given
string(0) ""
implode() expects parameter 2 to be array, string given
implode() expects parameter 2 to be array, string given
Done
