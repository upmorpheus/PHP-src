--TEST--
imagecreatefrompng() and empty/missing file
--EXTENSIONS--
gd
--SKIPIF--
<?php if (!function_exists("imagecreatefrompng")) print "skip"; ?>
--FILE--
<?php

$file = __DIR__."/001私はガラスを食べられます.test";
@unlink($file);

var_dump(imagecreatefrompng($file));
touch($file);
var_dump(imagecreatefrompng($file));

@unlink($file);

echo "Done\n";
?>
--EXPECTF--
Warning: imagecreatefrompng(%s001私はガラスを食べられます.test): Failed to open stream: No such file or directory in %s on line %d
bool(false)

Warning: imagecreatefrompng(): "%s001私はガラスを食べられます.test" is not a valid PNG file in %s on line %d
bool(false)
Done
