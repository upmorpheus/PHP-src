--TEST--
FFI 034: FFI::typeof(), FFI::sizeof(), FFI::alignof()
--EXTENSIONS--
ffi
--INI--
ffi.enable=1
--FILE--
<?php
$ffi = FFI::cdef();

$p1 = $ffi->new("uint8_t[2]");
$p2 = $ffi->new("uint16_t[2]");
$p3 = $ffi->new("uint32_t[2]");
var_dump(FFI::sizeof($p1), FFI::sizeof($p2), FFI::sizeof($p3));
var_dump(FFI::alignof($p1), FFI::alignof($p2), FFI::alignof($p3));
var_dump(FFI::sizeof(FFI::typeof($p1)), FFI::sizeof(FFI::typeof($p2)), FFI::sizeof(FFI::typeof($p3)));
var_dump(FFI::alignof(FFI::typeof($p1)), FFI::alignof(FFI::typeof($p2)), FFI::alignof(FFI::typeof($p3)));
?>
--EXPECT--
int(2)
int(4)
int(8)
int(1)
int(2)
int(4)
int(2)
int(4)
int(8)
int(1)
int(2)
int(4)
