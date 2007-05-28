--TEST--
Bug #39366 (imagerotate does not respect alpha with angles>45)
--SKIPIF--
<?php
	if (!extension_loaded('gd')) die("skip gd extension not available\n");
	if (!function_exists("imagerotate")) die("skip requires bundled GD library\n");
?>
--FILE--
<?php

$im = imagecreatetruecolor(10,10);
imagealphablending($im, 0);
imagefilledrectangle($im, 0,0, 8,8, 0x32FF0000);
$rotate = imagerotate($im, 180, 0);
imagecolortransparent($rotate,0);
imagesavealpha($rotate, true);
$c = imagecolorat($rotate,5,5);
printf("%X\n", $c);
?>
--EXPECTF--
32FF0000
