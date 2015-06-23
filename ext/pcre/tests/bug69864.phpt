--TEST--
Bug #69864 (Segfault in preg_replace_callback)
--FILE--
<?php
const PREG_CACHE_SIZE = 4096; // this has to be >= the resp. constant in php_pcre.c

var_dump(preg_replace_callback('/a/', function($m) {
    for ($i = 0; $i < PREG_CACHE_SIZE; $i++) {
        preg_match('/foo' . $i . 'bar/', '???foo' . $i . 'bar???');
    }
    return 'b';
}, 'aa'));
var_dump(preg_replace_callback('/a/', function($m) {
    for ($i = 0; $i < PREG_CACHE_SIZE; $i++) {
        preg_replace('/foo' . $i . 'bar/', 'baz', '???foo' . $i . 'bar???');
    }
    return 'b';
}, 'aa'));
var_dump(preg_replace_callback('/a/', function($m) {
    for ($i = 0; $i < PREG_CACHE_SIZE; $i++) {
        preg_split('/foo' . $i . 'bar/', '???foo' . $i . 'bar???');
    }
    return 'b';
}, 'aa'));
var_dump(preg_replace_callback('/a/', function($m) {
    for ($i = 0; $i < PREG_CACHE_SIZE; $i++) {
        preg_grep('/foo' . $i . 'bar/', ['???foo' . $i . 'bar???']);
    }
    return 'b';
}, 'aa'));
?>
--EXPECT--
string(2) "bb"
string(2) "bb"
string(2) "bb"
string(2) "bb"
