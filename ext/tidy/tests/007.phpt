--TEST--
Verbose tidy_setopt() / tidy_getopt()
--SKIPIF--
<?php if (!extension_loaded("Tidy")) print "skip"; ?>
--POST--
--GET--
--INI--
--FILE--
<?php 

    $tidy = tidy_create();
    echo "Current Value of 'tidy-mark': ";
    var_dump(tidy_getopt($tidy, "tidy-mark"));
    tidy_setopt($tidy, "tidy-mark", true);
    echo "\nNew Value of 'tidy-mark': ";
    var_dump(tidy_getopt($tidy, "tidy-mark"));
    echo "Current Value of 'error-file': ";
    var_dump(tidy_getopt($tidy, "error-file"));
    tidy_setopt($tidy, "error-file", "foobar");
    echo "\nNew Value of 'error-file': ";
    var_dump(tidy_getopt($tidy, "error-file"));
    echo "Current Value of 'tab-size': ";
    var_dump(tidy_getopt($tidy, "tab-size"));
    tidy_setopt($tidy, "tab-size", 10);
    echo "\nNew Value of 'tab-size': ";
    var_dump(tidy_getopt($tidy, "tab-size"));
?>
--EXPECT--
Current Value of 'tidy-mark': bool(false)

New Value of 'tidy-mark': bool(true)
Current Value of 'error-file': string(0) ""

New Value of 'error-file': string(6) "foobar"
Current Value of 'tab-size': int(8)

New Value of 'tab-size': int(10)