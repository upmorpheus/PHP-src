--TEST--
Bug #79067 (gdTransformAffineCopy() may use unitialized values)
--SKIPIF--
<?php
if (!extension_loaded('gd')) die('skip gd extension not available');
?>
--FILE--
<?php
$matrix = [1, 1, 1, 1, 1, 1];
$src = imagecreatetruecolor(8, 8);
var_dump(imageaffine($src, $matrix));
?>
--EXPECT--
bool(false)
