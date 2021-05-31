--TEST--
Test array_intersect() function : usage variations - assoc array with diff keys for 'arr1' argument
--FILE--
<?php
/*
 * Testing the functionality of array_intersect() by passing different
 * associative arrays having different possible keys to $arr1 argument.
 * The $arr2 argument is a fixed array
*/

echo "*** Testing array_intersect() : assoc array with diff keys to \$arr1 argument ***\n";

// get an unset variable
$unset_var = 10;
unset ($unset_var);

// get a heredoc string
$heredoc = <<<EOT
Hello world
EOT;

// different variations of associative arrays to be passed to $arr1 argument
$arrays = array (

       // empty array
/*1*/  array(),

       // arrays with integer keys
       array(0 => "0"),
       array(1 => "1"),
       array(1 => "1", 2 => "2", 3 => "3", 4 => "4"),

       // arrays with string keys
/*7*/  array('\tHello' => 111, 're\td' => "color",
             '\v\fworld' => 2.2, 'pen\n' => 33),
       array("\tHello" => 111, "re\td" => "color",
             "\v\fworld" => 2.2, "pen\n" => 33),
       array("hello", $heredoc => "string"), // heredoc

       // array with unset variable
/*10*/ array( @$unset_var => "hello"),

       // array with mixed keys
/*11*/ array('hello' => 1,  "fruit" => 2.2,
              133 => "int",
             @$unset_var => "unset", $heredoc => "heredoc")
);

// array to be passed to $arr2 argument
$arr2 = array(1, "float", "f4", "hello", 2.2, 'color', "string", "pen\n", 11);

// loop through each sub-array within $arrays to check the behavior of array_intersect()
$iterator = 1;
foreach($arrays as $arr1) {
  echo "-- Iterator $iterator --\n";

  // Calling array_intersect() with default arguments
  var_dump( array_intersect($arr1, $arr2) );

  // Calling array_intersect() with more arguments.
  // additional argument passed is the same as $arr1 argument
  var_dump( array_intersect($arr1, $arr2, $arr1) );
  $iterator++;
}

echo "Done";
?>
--EXPECT--
*** Testing array_intersect() : assoc array with diff keys to $arr1 argument ***
-- Iterator 1 --
array(0) {
}
array(0) {
}
-- Iterator 2 --
array(0) {
}
array(0) {
}
-- Iterator 3 --
array(1) {
  [1]=>
  string(1) "1"
}
array(1) {
  [1]=>
  string(1) "1"
}
-- Iterator 4 --
array(1) {
  [1]=>
  string(1) "1"
}
array(1) {
  [1]=>
  string(1) "1"
}
-- Iterator 5 --
array(2) {
  ["re\td"]=>
  string(5) "color"
  ["\v\fworld"]=>
  float(2.2)
}
array(2) {
  ["re\td"]=>
  string(5) "color"
  ["\v\fworld"]=>
  float(2.2)
}
-- Iterator 6 --
array(2) {
  ["re	d"]=>
  string(5) "color"
  ["world"]=>
  float(2.2)
}
array(2) {
  ["re	d"]=>
  string(5) "color"
  ["world"]=>
  float(2.2)
}
-- Iterator 7 --
array(2) {
  [0]=>
  string(5) "hello"
  ["Hello world"]=>
  string(6) "string"
}
array(2) {
  [0]=>
  string(5) "hello"
  ["Hello world"]=>
  string(6) "string"
}
-- Iterator 8 --
array(1) {
  [""]=>
  string(5) "hello"
}
array(1) {
  [""]=>
  string(5) "hello"
}
-- Iterator 9 --
array(2) {
  ["hello"]=>
  int(1)
  ["fruit"]=>
  float(2.2)
}
array(2) {
  ["hello"]=>
  int(1)
  ["fruit"]=>
  float(2.2)
}
Done
