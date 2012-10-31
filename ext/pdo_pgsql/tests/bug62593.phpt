--TEST--
PDO PgSQL Bug #62593 (Emulate prepares behave strangely with PARAM_BOOL)
--SKIPIF--
<?php
if (!extension_loaded('pdo') || !extension_loaded('pdo_pgsql')) die('skip not loaded');
require dirname(__FILE__) . '/config.inc';
require dirname(__FILE__) . '/../../../ext/pdo/tests/pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require dirname(__FILE__) . '/../../../ext/pdo/tests/pdo_test.inc';
$db = PDOTest::test_factory(dirname(__FILE__) . '/common.phpt');
$db->setAttribute(PDO::ATTR_EMULATE_PREPARES, true);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_SILENT);
$errors = array();

$value = true;
$query = $db->prepare('SELECT :foo IS FALSE as val_is_false');
$query->bindValue(':foo', $value, PDO::PARAM_BOOL);
$query->execute();
$errors[] = $query->errorInfo();
var_dump($value);

$query->bindValue(':foo', 0, PDO::PARAM_BOOL);
$query->execute();
$errors[] = $query->errorInfo();

// Verify bindParam maintains reference and only passes when execute is called
$value = true;
$query->bindParam(':foo', $value, PDO::PARAM_BOOL);
$value = false;
$query->execute();
$errors[] = $query->errorInfo();
var_dump($value);

$expect = 'No errors found';

foreach ($errors as $error)
{
  if (strpos('Invalid text representation', $error[2]) !== false)
  {
    $expect = 'Invalid boolean found';
  }
}
echo $expect;
?>
--EXPECTF--
bool(true)
bool(false)
No errors found
