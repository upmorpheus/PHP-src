--TEST--
double to string conversion tests (64bit)
--SKIPIF--
<?php if (PHP_INT_SIZE != 8) die("skip this test is for 64bit platform only"); ?>
--INI--
precision=14
--FILE--
<?php

$doubles = array(
        29000000000000000000000000000000000000,
        290000000000000000,
        290000000000000,
        29000000000000,
        29000000000000.123123,
        29000000000000.7123123,
        29000.7123123,
        239234242.7123123,
        0.12345678901234567890,
        10000000000000000000000000000000000000000000000,
        1000000000000000000000000000000000,
        100000000000000001,
        1000006000000000011,
        100000000000001,
        10000000000,
        999999999999999999,
        9999999999999999999,
        9999999999999999999999999999999999999,
        (float)0
        );

foreach ($doubles as $d) {
        var_dump((string)$d);
}

echo "Done\n";
?>
--EXPECT--
unicode(7) "2.9E+37"
unicode(18) "290000000000000000"
unicode(15) "290000000000000"
unicode(14) "29000000000000"
unicode(14) "29000000000000"
unicode(14) "29000000000001"
unicode(13) "29000.7123123"
unicode(15) "239234242.71231"
unicode(16) "0.12345678901235"
unicode(7) "1.0E+46"
unicode(7) "1.0E+33"
unicode(18) "100000000000000001"
unicode(19) "1000006000000000011"
unicode(15) "100000000000001"
unicode(11) "10000000000"
unicode(18) "999999999999999999"
unicode(7) "1.0E+19"
unicode(7) "1.0E+37"
unicode(1) "0"
Done
