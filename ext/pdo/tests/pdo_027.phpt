--TEST--
PDO Common: PDO_FETCH_LAZY
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
$dir = getenv('REDIR_TEST_DIR');
if (false == $dir) die('skip no driver');
require_once $dir . 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require getenv('REDIR_TEST_DIR') . 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec('create table test (id int, name varchar(10) NULL)');
$db->exec("INSERT INTO test (id,name) VALUES(1,'test1')");
$db->exec("INSERT INTO test (id,name) VALUES(2,'test2')");

foreach ($db->query("SELECT * FROM test", PDO_FETCH_LAZY) as $v) {
	echo $v->id.$v->name."\n";
}
?>
--EXPECT--
1test1
2test2
