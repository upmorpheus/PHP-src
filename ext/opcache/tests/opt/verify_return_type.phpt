--TEST--
Return type check elision
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.optimization_level=-1
opcache.opt_debug_level=0x20000
opcache.preload=
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

class Test1 {
    final public function getIntOrFloat(int $i): int|float {
        return $i;
    }
    final public function getInt(): int {
        return $this->getIntOrFloat();
    }
}

class Test2 {
    public function getInt(): int {
        return 42;
    }
    public function getInt2(): int {
        return $this->getInt();
    }
    public function getIntOrFloat(int $i): int|float {
        return $i;
    }
    public function getInt3(int $i): int {
        // Should not elide return type check. Test2::getIntOrFloat() returns only int,
        // but a child method may return int|float.
        return $this->getIntOrFloat($i);
    }
}

?>
--EXPECTF--
$_main:
     ; (lines=1, args=0, vars=0, tmps=0)
     ; (after optimizer)
     ; %s
0000 RETURN int(1)

Test1::getIntOrFloat:
     ; (lines=2, args=1, vars=1, tmps=0)
     ; (after optimizer)
     ; %s
0000 CV0($i) = RECV 1
0001 RETURN CV0($i)

Test1::getInt:
     ; (lines=3, args=0, vars=0, tmps=1)
     ; (after optimizer)
     ; %s
0000 INIT_METHOD_CALL 0 THIS string("getIntOrFloat")
0001 V0 = DO_UCALL
0002 RETURN V0

Test2::getInt:
     ; (lines=1, args=0, vars=0, tmps=0)
     ; (after optimizer)
     ; %s
0000 RETURN int(42)

Test2::getInt2:
     ; (lines=3, args=0, vars=0, tmps=1)
     ; (after optimizer)
     ; %s
0000 INIT_METHOD_CALL 0 THIS string("getInt")
0001 V0 = DO_FCALL
0002 RETURN V0

Test2::getIntOrFloat:
     ; (lines=2, args=1, vars=1, tmps=0)
     ; (after optimizer)
     ; %s
0000 CV0($i) = RECV 1
0001 RETURN CV0($i)

Test2::getInt3:
     ; (lines=6, args=1, vars=1, tmps=1)
     ; (after optimizer)
     ; %s
0000 CV0($i) = RECV 1
0001 INIT_METHOD_CALL 1 THIS string("getIntOrFloat")
0002 SEND_VAR CV0($i) 1
0003 V1 = DO_FCALL
0004 VERIFY_RETURN_TYPE V1
0005 RETURN V1
