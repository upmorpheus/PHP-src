--TEST--
strip comments and whitespace with -w
--SKIPIF--
<?php 
include "skipif.inc"; 
if (substr(PHP_OS, 0, 3) == 'WIN') {
	die ("skip not for Windows");
}
?>
--FILE--
<?php

$php = $_ENV['TEST_PHP_EXECUTABLE'];

$filename = dirname(__FILE__).'/007.test.php';
$code ='
<?php
/* some test script */

class test { /* {{{ */
	public $var = "test"; //test var
#perl style comment 
	private $pri; /* private attr */

	function foo(/* void */) {
	}
}
/* }}} */

?>
';

file_put_contents($filename, $code);

var_dump(`"$php" -w "$filename"`);
var_dump(`"$php" -w "wrong"`);
var_dump(`echo "<?php /* comment */ class test {\n // comment \n function foo() {} } ?>" | $php -w`);

@unlink($filename);

echo "Done\n";
?>
--EXPECTF--	
string(81) "
<?php
 class test { public $var = "test"; private $pri; function foo() { } } ?>
"
string(33) "Could not open input file: wrong
"
string(43) "<?php  class test { function foo() {} } ?>
"
Done
