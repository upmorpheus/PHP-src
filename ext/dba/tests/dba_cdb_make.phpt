--TEST--
DBA CDB_MAKE handler test
--SKIPIF--
<?php
    $handler = 'cdb_make';
    require_once(__DIR__ .'/skipif.inc');
    die('info CDB_MAKE does not support reading');
?>
--FILE--
<?php
    $handler = 'cdb_make';
    require_once(__DIR__ .'/test.inc');
    echo "database handler: $handler\n";
    // print md5 checksum of test.cdb which is generated by cdb_make program
    var_dump(md5_file(__DIR__.'/test.cdb'));
    if (($db_make=dba_open($db_file, "n", $handler))!==FALSE) {
        dba_insert("1", "1", $db_make);
        dba_insert("2", "2", $db_make);
        dba_insert("1", "3", $db_make);
        dba_insert("2", "1", $db_make);
        dba_insert("3", "3", $db_make);
        dba_insert("1", "2", $db_make);
        dba_insert("4", "4", $db_make);
//		dba_replace cdb_make doesn't know replace
        dba_close($db_make);
        // write md5 checksum of generated database file
        var_dump(md5_file($db_file));
        // no need to test created database: this is done by dba_cdb_read.phpt
    } else {
        echo "Error creating database\n";
    }
?>
--CLEAN--
<?php
    require(__DIR__ .'/clean.inc');
?>
--EXPECT--
database handler: cdb_make
string(32) "12fc5ba2b9dcfef2480e5324eeb5f3e5"
string(32) "12fc5ba2b9dcfef2480e5324eeb5f3e5"
