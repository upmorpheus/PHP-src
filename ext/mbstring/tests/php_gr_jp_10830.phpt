--TEST--
php-users@php.gr.jp #10830 
--SKIPIF--
<?php
extension_loaded('mbstring') or die('skip mbstring not available');
function_exists('mb_ereg') or die("SKIP");
?>
--FILE--
<?php
$a="aaa\n<>";

var_dump( mb_ereg("^[^><]+$",$a) );
var_dump( ereg("^[^><]+$",$a) );
?>

--EXPECT--
bool(false)
bool(false)
