--TEST--
execute a file with -f
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

$filename = dirname(__FILE__).'/008.test.php';
$code ='
<?php

class test { 
	private $pri; 
}

var_dump(test::$pri);
?>
';

file_put_contents($filename, $code);

var_dump(`$php -f "$filename"`);
var_dump(`$php -f "wrong"`);

@unlink($filename);

echo "Done\n";
?>
--EXPECTF--	
string(%d) "

Fatal error: Cannot access private property test::$pri in %s on line %d
"
Could not open input file: wrong
NULL
Done
