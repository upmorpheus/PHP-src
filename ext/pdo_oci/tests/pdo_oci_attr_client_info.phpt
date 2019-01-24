--TEST--
PDO_OCI: Attribute: Setting session client info
--SKIPIF--
<?php
if (!extension_loaded('pdo') || !extension_loaded('pdo_oci')) die('skip not loaded');
require(dirname(__FILE__).'/../../pdo/tests/pdo_test.inc');
PDOTest::skip();
?>
--FILE--
<?php

require(dirname(__FILE__) . '/../../pdo/tests/pdo_test.inc');

$query = 'select client_info from v$session where sid = (select distinct sid from v$mystat)';

$dbh = PDOTest::factory();

$stmt = $dbh->query($query);
$row = $stmt->fetch();
echo 'CLIENT_INFO NOT SET: ';
var_dump($row['client_info']);

$dbh->setAttribute(PDO::OCI_ATTR_CLIENT_INFO, "some client info");

$stmt = $dbh->query($query);
$row = $stmt->fetch();
echo 'CLIENT_INFO SET: ';
var_dump($row['client_info']);

$dbh->setAttribute(PDO::OCI_ATTR_CLIENT_INFO, "something else!");

$stmt = $dbh->query($query);
$row = $stmt->fetch();
echo 'CLIENT_INFO RESET: ';
var_dump($row['client_info']);

$dbh->setAttribute(PDO::OCI_ATTR_CLIENT_INFO, null);

$stmt = $dbh->query($query);
$row = $stmt->fetch();
echo 'CLIENT_INFO NULLED: ';
var_dump($row['client_info']);

echo "Done\n";

?>
--EXPECT--
CLIENT_INFO NOT SET: NULL
CLIENT_INFO SET: string(16) "some client info"
CLIENT_INFO RESET: string(15) "something else!"
CLIENT_INFO NULLED: NULL
Done
