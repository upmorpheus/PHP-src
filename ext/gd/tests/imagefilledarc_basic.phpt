--TEST--
Testing imagefilledarc() of GD library
--CREDITS--
Edgar Ferreira da Silva <contato [at] edgarfs [dot] com [dot] br>
#testfest PHPSP on 2009-06-20
--SKIPIF--
<?php 
if (!extension_loaded("gd")) die("skip GD not present");
if (!GD_BUNDLED && version_compare(GD_VERSION, '2.2.2', '<')) {
	die("skip test requires GD 2.2.2 or higher");
}
?>
--FILE--
<?php

$image = imagecreatetruecolor(100, 100);

$white = imagecolorallocate($image, 0xFF, 0xFF, 0xFF);

//create an arc and fill it with white color    
imagefilledarc($image, 50, 50, 30, 30, 0, 90, $white, IMG_ARC_PIE);

ob_start();
imagegd($image);
$img = ob_get_contents();
ob_end_clean();

echo md5(base64_encode($img));
?>
--EXPECT--
87637c60ac0ceea48dbcaa4d98319f90
