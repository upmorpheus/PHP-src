--TEST--
PDO Common: PDO_FETCH_BOTH
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) print 'skip';
if (false == getenv('REDIR_TEST_DIR')) print 'skip no driver'; ?>
--FILE--
<?php
require getenv('REDIR_TEST_DIR') . 'pdo_test.php';
$db = PDOTest::factory();

$db->exec('CREATE TABLE test(id int NOT NULL PRIMARY KEY, val VARCHAR(10))');
$db->exec("INSERT INTO test VALUES(1, 'A')");
$db->exec("INSERT INTO test VALUES(2, 'B')");
$db->exec("INSERT INTO test VALUES(3, 'C')");

$stmt = $db->prepare('SELECT * from test');
$stmt->execute();

var_dump($stmt->fetchAll(PDO_FETCH_BOTH));
?>
--EXPECT--
array(3) {
  [0]=>
  array(4) {
    ["id"]=>
    string(1) "1"
    [0]=>
    string(1) "1"
    ["val"]=>
    string(1) "A"
    [1]=>
    string(1) "A"
  }
  [1]=>
  array(4) {
    ["id"]=>
    string(1) "2"
    [0]=>
    string(1) "2"
    ["val"]=>
    string(1) "B"
    [1]=>
    string(1) "B"
  }
  [2]=>
  array(4) {
    ["id"]=>
    string(1) "3"
    [0]=>
    string(1) "3"
    ["val"]=>
    string(1) "C"
    [1]=>
    string(1) "C"
  }
}
