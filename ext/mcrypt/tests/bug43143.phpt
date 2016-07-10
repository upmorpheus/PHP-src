--TEST--
Bug #43143 (Warning about empty IV with MCRYPT_MODE_ECB)
--SKIPIF--
<?php if (!extension_loaded("mcrypt")) print "skip"; 
if (!extension_loaded("hash")) print "skip"; ?>
--FILE--
<?php
echo "ECB\n";
$input = 'to be encrypted';
$mkey = hash('sha256', 'secret key', TRUE);
$data = mcrypt_encrypt(MCRYPT_RIJNDAEL_256, $mkey, $input, MCRYPT_MODE_ECB);
echo "CFB\n";
$input = 'to be encrypted';
$mkey = hash('sha256', 'secret key', TRUE);
$data = mcrypt_encrypt(MCRYPT_RIJNDAEL_256, $mkey, $input, MCRYPT_MODE_CFB);
echo "END\n";
?>
--EXPECTF--
ECB

Deprecated: mcrypt_encrypt(): The mcrypt extension is deprecated and will be removed in the future: use openssl instead in %s%ebug43143.php on line 5
CFB

Deprecated: mcrypt_encrypt(): The mcrypt extension is deprecated and will be removed in the future: use openssl instead in %s%ebug43143.php on line 9

Warning: mcrypt_encrypt(): Encryption mode requires an initialization vector of size 32 in %sbug43143.php on line 9
END
