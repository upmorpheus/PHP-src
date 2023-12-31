--TEST--
Check various LOB error messages
--EXTENSIONS--
oci8
--SKIPIF--
<?php
require_once 'skipifconnectfailure.inc';
$target_dbs = array('oracledb' => true, 'timesten' => false);  // test runs on these DBs
require __DIR__.'/skipif.inc';
?>
--FILE--
<?php

require __DIR__.'/connect.inc';
require __DIR__.'/create_table.inc';

$ora_sql = "INSERT INTO
                       ".$schema.$table_name." (blob)
                      VALUES (empty_blob())
                      RETURNING
                               blob
                      INTO :v_blob ";

$statement = oci_parse($c,$ora_sql);
$blob = oci_new_descriptor($c,OCI_D_LOB);
oci_bind_by_name($statement,":v_blob", $blob,-1,OCI_B_BLOB);
oci_execute($statement, OCI_DEFAULT);

var_dump($blob);

var_dump($blob->writeTemporary("test", OCI_D_LOB));

$str = "string";
var_dump($blob->write($str));
var_dump($blob->truncate(1));
var_dump($blob->truncate(1));
var_dump($blob->truncate(2));
var_dump($blob->read(2));

var_dump($blob->import("does_not_exist"));
var_dump($blob->saveFile("does_not_exist"));

try {
    var_dump($blob->truncate(-1));
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}

require __DIR__.'/drop_table.inc';

echo "Done\n";

?>
--EXPECTF--
object(OCILob)#%d (1) {
  ["descriptor"]=>
  resource(%d) of type (oci8 descriptor)
}

Warning: OCILob::writeTemporary(): Invalid temporary lob type: %d in %s on line %d
bool(false)
int(6)
bool(true)
bool(true)

Warning: OCILob::truncate(): Size must be less than or equal to the current LOB size in %s on line %d
bool(false)

Warning: OCILob::read(): Offset must be less than size of the LOB in %s on line %d
bool(false)

Warning: OCILob::import(): Can't open file %s in %s on line %d
bool(false)

Warning: OCILob::saveFile(): Can't open file %s in %s on line %d
bool(false)
OCILob::truncate(): Argument #1 ($length) must be greater than or equal to 0
Done
