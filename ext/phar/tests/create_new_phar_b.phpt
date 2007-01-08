--TEST--
Phar: create a completely new phar
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
--INI--
phar.readonly=1
phar.require_hash=1
--FILE--
<?php

file_put_contents('phar://' . dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php/a.php',
	'brand new!');
include 'phar://' . dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php/a.php';
?>

===DONE===
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--

Warning: file_put_contents(phar://%screate_new_phar_b.phar.php/a.php): failed to open stream: No such file or directory in %screate_new_phar_b.php on line %d

Warning: include(%screate_new_phar_b.phar.php): failed to open stream: No such file or directory in %screate_new_phar_b.php on line %d

Warning: include(phar://%screate_new_phar_b.phar.php/a.php): failed to open stream: No such file or directory in %screate_new_phar_b.php on line %d

Warning: include(): Failed opening 'phar://%screate_new_phar_b.phar.php/a.php' for inclusion (include_path='.') in %screate_new_phar_b.php on line %d

===DONE===
