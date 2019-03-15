--TEST--
Phar: fopen a .phar for writing (existing file)
--SKIPIF--
<?php
if (!extension_loaded("phar")) die("skip");
?>
--INI--
phar.readonly=1
phar.require_hash=0
--FILE--
<?php
$fname = __DIR__ . '/' . basename(__FILE__, '.php') . '.phar.php';
$pname = 'phar://' . $fname;
$file = "<?php __HALT_COMPILER(); ?>";

$files = array();
$files['a.php'] = '<?php echo "This is a\n"; ?>';
$files['b.php'] = '<?php echo "This is b\n"; ?>';
$files['b/c.php'] = '<?php echo "This is b/c\n"; ?>';
include 'files/phar_test.inc';

$fp = fopen($pname . '/b/c.php', 'wb');
fwrite($fp, 'extra');
fclose($fp);
include $pname . '/b/c.php';
?>
===DONE===
--CLEAN--
<?php unlink(__DIR__ . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
Warning: fopen(phar://%sopen_for_write_existing_c.phar.php/b/c.php): failed to open stream: phar error: write operations disabled by the php.ini setting phar.readonly in %sopen_for_write_existing_c.php on line %d

Warning: fwrite() expects parameter 1 to be resource, bool given in %spen_for_write_existing_c.php on line %d

Warning: fclose() expects parameter 1 to be resource, bool given in %spen_for_write_existing_c.php on line %d
This is b/c
===DONE===
