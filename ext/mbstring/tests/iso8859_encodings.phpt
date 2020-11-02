--TEST--
Exhaustive test of verification and conversion of ISO-8859-X text
--SKIPIF--
<?php
extension_loaded('mbstring') or die('skip mbstring not available');
if (getenv("SKIP_SLOW_TESTS")) die("skip slow test");
?>
--FILE--
<?php
include('encoding_tests.inc');

for ($n = 1; $n <= 16; $n++) {
    if ($n == 11 || $n == 12)
        continue;
    testEncodingFromUTF16ConversionTable(__DIR__ . "/data/8859-$n.txt", "ISO-8859-{$n}");
}
?>
--EXPECT--
Tested ISO-8859-1 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-1
Tested ISO-8859-2 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-2
Tested ISO-8859-3 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-3
Tested ISO-8859-4 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-4
Tested ISO-8859-5 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-5
Tested ISO-8859-6 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-6
Tested ISO-8859-7 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-7
Tested ISO-8859-8 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-8
Tested ISO-8859-9 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-9
Tested ISO-8859-10 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-10
Tested ISO-8859-13 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-13
Tested ISO-8859-14 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-14
Tested ISO-8859-15 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-15
Tested ISO-8859-16 -> UTF-16BE
Tested UTF-16BE -> ISO-8859-16
