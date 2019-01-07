--TEST--
Typed property must be compatible when returned via &__get()
--FILE--
<?php
  
class Test {
	public $prop = "x";
	public int $val;

	public function &__get($name) {
		return $this->prop;
	}
}

$test = new Test;
$dummyRef = &$test->prop;
var_dump($test);
try {
	var_dump($test->val);
} catch (TypeError $e) { print $e->getMessage()."\n"; }
var_dump($test);

$test->prop = "y";
var_dump($test->prop);

?>
--EXPECT--
object(Test)#1 (1) {
  ["prop"]=>
  &string(1) "x"
  ["val"]=>
  uninitialized(int)
}
Typed property Test::$val must be int, string used
object(Test)#1 (1) {
  ["prop"]=>
  &string(1) "x"
  ["val"]=>
  uninitialized(int)
}
string(1) "y"
