--TEST--
Testing openssl_x509_digest()
--SKIPIF--
<?php
if (!extension_loaded("openssl")) die("skip");
?>
--FILE--
<?php

$cert = "file://" . dirname(__FILE__) . "/cert.crt";

echo "** Testing with no parameters **\n";
var_dump(openssl_x509_digest());

echo "** Testing default functionality **\n";
var_dump(openssl_x509_digest($cert));

echo "** Testing hash method md5 **\n";
var_dump(openssl_x509_digest($cert, 'md5'));

echo "**Testing raw output md5 **\n";
var_dump(bin2hex(openssl_x509_digest($cert, 'md5', true)));

echo "** Testing bad certification **\n";
var_dump(openssl_x509_digest('123'));
echo "** Testing bad hash method **\n";
var_dump(openssl_x509_digest($cert, 'xx45'));
--EXPECTF--
** Testing with no parameters **

Warning: openssl_x509_digest() expects at least 1 parameter, 0 given in %s on line %d
NULL
** Testing default functionality **
string(40) "6e6fd1ea10a5a23071d61c728ee9b40df6dbc33c"
** Testing hash method md5 **
string(32) "ac77008e172897e06c0b065294487a67"
**Testing raw output md5 **
string(32) "ac77008e172897e06c0b065294487a67"
** Testing bad certification **

Warning: openssl_x509_digest(): cannot get cert from parameter 1 in %s on line %d
bool(false)
** Testing bad hash method **

Warning: openssl_x509_digest(): Unknown signature algorithm in %s on line %d
bool(false)

