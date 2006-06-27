--TEST--
each() tests
--FILE--
<?php

var_dump(each());
$var = 1;
var_dump(each($var));
$var = "string";
var_dump(each($var));
$var = array(1,2,3);
var_dump(each($var));
$var = array("a"=>1,"b"=>2,"c"=>3);
var_dump(each($var));

$a = array(1);
$a [] =&$a[0];

var_dump(each($a));


echo "Done\n";
?>
--EXPECTF--	
Warning: Wrong parameter count for each() in %s on line %d
NULL

Warning: Variable passed to each() is not an array or object in %s on line %d
NULL

Warning: Variable passed to each() is not an array or object in %s on line %d
NULL
array(4) {
  [1]=>
  int(1)
  ["value"]=>
  int(1)
  [0]=>
  int(0)
  ["key"]=>
  int(0)
}
array(4) {
  [1]=>
  int(1)
  ["value"]=>
  int(1)
  [0]=>
  string(1) "a"
  ["key"]=>
  string(1) "a"
}
array(4) {
  [1]=>
  int(1)
  ["value"]=>
  int(1)
  [0]=>
  int(0)
  ["key"]=>
  int(0)
}
Done
--UEXPECTF--
Warning: Wrong parameter count for each() in %s on line %d
NULL

Warning: Variable passed to each() is not an array or object in %s on line %d
NULL

Warning: Variable passed to each() is not an array or object in %s on line %d
NULL
array(4) {
  [1]=>
  int(1)
  [u"value"]=>
  int(1)
  [0]=>
  int(0)
  [u"key"]=>
  int(0)
}
array(4) {
  [1]=>
  int(1)
  [u"value"]=>
  int(1)
  [0]=>
  unicode(1) "a"
  [u"key"]=>
  unicode(1) "a"
}
array(4) {
  [1]=>
  int(1)
  [u"value"]=>
  int(1)
  [0]=>
  int(0)
  [u"key"]=>
  int(0)
}
Done
