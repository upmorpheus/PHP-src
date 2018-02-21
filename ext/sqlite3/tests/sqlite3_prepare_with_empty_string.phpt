--TEST--
SQLite3::prepare test with empty string argument
--CREDITS--
Thijs Feryn <thijs@feryn.eu>
#TestFest PHPBelgium 2009
--SKIPIF--
<?php require_once(__DIR__ . '/skipif.inc'); ?>
--FILE--
<?php
$db = new SQLite3(':memory:');
var_dump($db->prepare(''));
echo "Done\n";
?>
--EXPECT--
bool(false)
Done
