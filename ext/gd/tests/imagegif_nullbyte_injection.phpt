--TEST--
Testing null byte injection in imagegif
--SKIPIF--
<?php
if(!extension_loaded('gd')){ die('skip gd extension not available'); }
?>
--FILE--
<?php
$image = imagecreate(1,1);// 1px image
try {
    imagegif($image, "./foo\0bar");
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
?>
--EXPECT--
Invalid 2nd parameter, filename must not contain null bytes
