--TEST--
PDO_DBLIB: Does not support get column meta
--SKIPIF--
<?php
if (!extension_loaded('pdo') || !extension_loaded('pdo_dblib')) die('skip not loaded');
require dirname(__FILE__) . '/config.inc';
require dirname(__FILE__) . '/../../../ext/pdo/tests/pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require dirname(__FILE__) . '/../../../ext/pdo/tests/pdo_test.inc';
$db = PDOTest::test_factory(dirname(__FILE__) . '/common.phpt');
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
$db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, false);

$stmt = $db->prepare("select ic1.* from information_schema.columns ic1");
$stmt->execute();
var_dump($stmt->getColumnMeta(0));
$stmt = null;
?>
--EXPECT--
array(8) {
  ["max_length"]=>
  int(255)
  ["precision"]=>
  int(0)
  ["scale"]=>
  int(0)
  ["column_source"]=>
  string(13) "table_catalog"
  ["native_type"]=>
  string(4) "char"
  ["name"]=>
  string(13) "table_catalog"
  ["len"]=>
  int(255)
  ["pdo_type"]=>
  int(2)
}
