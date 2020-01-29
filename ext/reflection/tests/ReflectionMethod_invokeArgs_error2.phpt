--TEST--
ReflectionMethod::invokeArgs() further errors
--FILE--
<?php

class TestClass {

    public function foo() {
        echo "Called foo()\n";
        var_dump($this);
        return "Return Val";
    }
}

$foo = new ReflectionMethod('TestClass', 'foo');

$testClassInstance = new TestClass();

try {
    var_dump($foo->invokeArgs($testClassInstance, true));
} catch (Error $e) {
    var_dump($e->getMessage());
}

?>
--EXPECT--
string(90) "ReflectionMethod::invokeArgs() expects argument #2 ($args) to be of type array, bool given"
