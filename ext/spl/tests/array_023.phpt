--TEST--
Testing class extending to ArrayObject and serialize
--FILE--
<?php

class Name extends ArrayObject
{
    public $var = 'a';
    protected $bar = 'b';
    private $foo = 'c';
}

$a = new Name();
var_dump($a);
var_dump($a->var);

$a = unserialize(serialize($a));

var_dump($a);
var_dump($a->var);

?>
--EXPECT--
object(Name)#1 (4) {
  ["var"]=>
  string(1) "a"
  ["bar":protected]=>
  string(1) "b"
  ["foo":"Name":private]=>
  string(1) "c"
  ["storage":"ArrayObject":private]=>
  array(0) {
  }
}
string(1) "a"
object(Name)#2 (4) {
  ["var"]=>
  string(1) "a"
  ["bar":protected]=>
  string(1) "b"
  ["foo":"Name":private]=>
  string(1) "c"
  ["storage":"ArrayObject":private]=>
  array(0) {
  }
}
string(1) "a"
