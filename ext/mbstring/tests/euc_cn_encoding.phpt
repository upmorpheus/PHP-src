--TEST--
Exhaustive test of verification and conversion of EUC-CN text
--EXTENSIONS--
mbstring
--SKIPIF--
<?php
if (getenv("SKIP_SLOW_TESTS")) die("skip slow test");
?>
--FILE--
<?php
include('encoding_tests.inc');
testEncodingFromUTF16ConversionTable(__DIR__ . '/data/EUC-CN.txt', 'EUC-CN');
?>
--EXPECT--
Tested EUC-CN -> UTF-16BE
Tested UTF-16BE -> EUC-CN
