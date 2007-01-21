--TEST--
Phar::mapPhar buffer overrun
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip";?>
--INI--
phar.require_hash=0
--FILE--
<?php
$file = "<?php
Phar::mapPhar('hio');
__HALT_COMPILER(); ?>";

// this fails because the manifest length does not include the other 10 byte manifest data

$manifest = pack('V', 1) . 'a' . pack('VVVVVV', 0, time(), 0, crc32(''), 0x00000000, 0);
$file .= pack('VVnVV', strlen($manifest), 1, 0x0900, 0x00000000, 3) . 'hio' . $manifest;

file_put_contents(dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php', $file);
include dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php';
echo file_get_contents('phar://hio/a');
?>
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
%satal error: Phar::mapPhar(): internal corruption of phar "%s" (too many manifest entries for size of manifest) in %s on line %d
