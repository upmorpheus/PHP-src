--TEST--
Phar: PHP bug #72321: invalid free in phar_extract_file()
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--FILE--
<?php
chdir(__DIR__);
mkdir("test72321");
$phar = new PharData("72321_1.zip");
$phar->extractTo("test72321");
$phar = new PharData("72321_2.zip");
try {
$phar->extractTo("test72321");
} catch(PharException $e) {
	print $e->getMessage()."\n";
}
?>
DONE
--CLEAN--
<?php unlink(__DIR__."/test72321/AAAAAAAAxxxxBBBBCCCCCCCCxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
rmdir(__DIR__."/test72321");
?>
--EXPECTF--
Warning: PharData::extractTo(): Not a directory in %s/bug72321.php on line %d
Extraction from phar "%s/72321_2.zip" failed: Cannot extract "AAAAAAAAxxxxBBBBCCCCCCCCxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/b/c", could not create directory "test72321/AAAAAAAAxxxxBBBBCCCCCCCCxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/b"
DONE