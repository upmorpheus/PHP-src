--TEST--
Bug #72697: select_colors write out-of-bounds
--SKIPIF--
<?php
if (!function_exists("imagecreatetruecolor")) die("skip");
if (PHP_INT_MAX !== 9223372036854775807) die("skip for 64-bit long systems only");
?>
--FILE--
<?php

require __DIR__ . '/func.inc';

$img=imagecreatetruecolor(10, 10);

trycatch_dump(
    fn() => imagetruecolortopalette($img, false, PHP_INT_MAX / 8)
);

?>
DONE
--EXPECT--
!! [Error] Number of colors has to be greater than zero and no more than 2147483647
DONE
