--TEST--
Catchable fatal error [2]
--FILE--
<?php
	class Foo {
	}

	function blah (Foo $a)
	{
	}

	function error()
	{
		$a = func_get_args();
		var_dump($a);
	}

	set_error_handler('error');

	blah (new StdClass);
	echo "ALIVE!\n";
?>
--EXPECTF--
array(5) {
  [0]=>
  int(4096)
  [1]=>
  string(%d) "Argument 1 passed to blah() must be an instance of Foo, called in %scatchable_error_002.php on line 17 and defined"
  [2]=>
  string(%d) "%scatchable_error_002.php"
  [3]=>
  int(5)
  [4]=>
  array(0) {
  }
}
ALIVE!
--UEXPECTF--
array(5) {
  [0]=>
  int(4096)
  [1]=>
  unicode(%d) "Argument 1 passed to blah() must be an instance of Foo, called in %scatchable_error_002.php on line 17 and defined"
  [2]=>
  unicode(%d) "%scatchable_error_002.php"
  [3]=>
  int(5)
  [4]=>
  array(0) {
  }
}
ALIVE!
