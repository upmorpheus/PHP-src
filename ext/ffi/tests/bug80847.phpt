--TEST--
Bug #80847 (Nested structs)
--SKIPIF--
<?php
if (!extension_loaded('ffi')) die('skip ffi extension not available');
if (!extension_loaded('zend_test')) die('skip zend_test extension not available');
?>
--FILE--
<?php
require_once('utils.inc');
$header = <<<HEADER
    typedef struct bug80847_01 {
        uint64_t b;
        double c;
    } bug80847_01;

    typedef struct bug80847_02 {
        bug80847_01 a;
    } bug80847_02;

	bug80847_02 ffi_bug80847(bug80847_02 s);
HEADER;

if (PHP_OS_FAMILY !== 'Windows') {
    $ffi = FFI::cdef($header);
} else {
    try {
        $ffi = FFI::cdef($header, 'php_zend_test.dll');
    } catch (FFI\Exception $ex) {
        $ffi = FFI::cdef($header, ffi_get_php_dll_name());
    }
}
$x = $ffi->new('bug80847_02');
$x->a->b = 42;
$x->a->c = 42.5;
var_dump($x);
$y = $ffi->ffi_bug80847($x);
var_dump($y);
?>
--EXPECTF--
object(FFI\CData:struct bug80847_02)#%d (1) {
  ["a"]=>
  object(FFI\CData:struct bug80847_01)#%d (2) {
    ["b"]=>
    int(42)
    ["c"]=>
    float(42.5)
  }
}
object(FFI\CData:struct bug80847_02)#%d (1) {
  ["a"]=>
  object(FFI\CData:struct bug80847_01)#%d (2) {
    ["b"]=>
    int(52)
    ["c"]=>
    float(32.5)
  }
}
