--TEST--
Various fgetcsv() error conditions
--FILE--
<?php

$file_name = __DIR__ . '/fgetcsv_error_conditions.csv';
$file_handle = fopen($file_name, 'r');

$length = 1024;
$delimiter = ',';
$enclosure = '"';

echo 'fgetcsv() with negative length' . \PHP_EOL;
try {
    var_dump( fgetcsv($file_handle, -10) );
} catch (\ValueError $e) {
    echo $e->getMessage() . \PHP_EOL;
}
try {
    var_dump( fgetcsv($file_handle, -10, $delimiter) );
} catch (\ValueError $e) {
    echo $e->getMessage() . \PHP_EOL;
}
try {
    var_dump( fgetcsv($file_handle, -10, $delimiter, $enclosure) );
} catch (\ValueError $e) {
    echo $e->getMessage() . \PHP_EOL;
}

echo 'fgetcsv() with delimiter as NULL' . \PHP_EOL;
try {
    var_dump( fgetcsv($file_handle, $length, NULL, $enclosure) );
} catch (\ValueError $e) {
    echo $e->getMessage() . \PHP_EOL;
}

echo 'fgetcsv() with enclosure as NULL' . \PHP_EOL;
try {
    var_dump( fgetcsv($file_handle, $length, $delimiter, NULL) );
} catch (\ValueError $e) {
    echo $e->getMessage() . \PHP_EOL;
}

echo 'fgetcsv() with delimiter & enclosure as NULL' . \PHP_EOL;
try {
    var_dump( fgetcsv($file_handle, $length, NULL, NULL) );
} catch (\ValueError $e) {
    echo $e->getMessage() . \PHP_EOL;
}
?>
--EXPECT--
fgetcsv() with negative length
Length parameter may not be negative
Length parameter may not be negative
Length parameter may not be negative
fgetcsv() with delimiter as NULL
delimiter must be a character
fgetcsv() with enclosure as NULL
enclosure must be a character
fgetcsv() with delimiter & enclosure as NULL
delimiter must be a character
