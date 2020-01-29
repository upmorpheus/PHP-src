--TEST--
Testing null byte injection in imagegd
--SKIPIF--
<?php
        if(!extension_loaded('gd')){ die('skip gd extension not available'); }
?>
--FILE--
<?php
$image = imagecreate(1,1);// 1px image
try {
    imagegd($image, "./foo\0bar");
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
?>
--EXPECT--
imagegd() expects argument #2 ($to) to be a valid path, string given
