--TEST--
Bug #72913 (imagecopy() loses single-color transparency on palette images)
--SKIPIF--
<?php
if (!extension_loaded('gd')) die('skip gd extension not available');
?>
--FILE--
<?php
$base64 = 'iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAIAAACRXR/mAAAABnRSTlMAAAAAAABu'
    . 'pgeRAAAAVklEQVRYw+3UQQqAMBAEwf3/p9eTBxEPiWAmWMU8oGFJqgAAuOpzWTX3'
    . 'xQUti+uRJTZ9V5aY1bOTFZLV7yZr9zt6ibv/qPXfrMpsGipbIy7oqQ8AYJED1plD'
    . 'y5PCu2sAAAAASUVORK5CYII=';
$src = imagecreatefromstring(base64_decode($base64));

$dst = imagecreate(50, 50);
$transparent = imagecolorallocatealpha($dst, 255, 255, 255, 127);
imagealphablending($dst, false);
imagesavealpha($dst, true);

imagecopy($dst, $src, 0,0, 0,0, 50,50);

ob_start();
imagegd($dst);
echo md5(ob_get_clean()), PHP_EOL;
?>
==DONE==
--EXPECT--
f03c27f20710e21debd7090c660f1a1e
==DONE==
