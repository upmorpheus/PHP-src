--TEST--
iterable type#005 - Return type covariance
--FILE--
<?php

class Test {
    function method(): iterable {
        return [];
    }
}

class TestArray extends Test {
    function method(): array {
        return [];
    }
}

class TestTraversable extends Test {
    function method(): Traversable {
        return new ArrayIterator([]);
    }
}

class TestIterator extends Test {
    function method(): Iterator {
        return new ArrayIterator([]);
    }
}

class TestGenerator extends Test {
    function method(): Generator {
        return (function () { yield; })();
    }
}

class TestScalar extends Test {
    function method(): int {
        return 1;
    }
}

?>
--EXPECTF--

Fatal error: Declaration of TestScalar::method(): int must be compatible with Test::method(): iterable in %s on line %d
