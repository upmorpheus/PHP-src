--TEST--
register_shutdown_function() & __call
--FILE--
<?php
class test {
        function _foo() {
                throw new Exception('test');
        }
        function __call($name=null, $args=null) {
                return test::_foo();
        }
}

var_dump(register_shutdown_function(array("test","__call")));

echo "Done\n";
?>
--EXPECTF--	
Strict Standards: Non-static method test::__call() cannot be called statically in %s on line %d
NULL
Done

Strict Standards: Non-static method test::__call() cannot be called statically in Unknown on line 0

Fatal error: Non-static method test::__call() cannot be called statically in Unknown on line 0
