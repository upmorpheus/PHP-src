--TEST--
Observer: Basic observability of functions only (with run-time swapping)
--EXTENSIONS--
zend_test
--INI--
zend_test.observer.enabled=1
zend_test.observer.observe_function_names=foo
--FILE--
<?php
function foo()
{
    echo 'Foo' . PHP_EOL;
}

function bar()
{
    echo 'Bar' . PHP_EOL;
}

foo();
bar();

ini_set("zend_test.observer.observe_function_names", "bar");

foo();
bar();

?>
--EXPECTF--
<!-- init '%s%eobserver_basic_06.php' -->
<!-- init foo() -->
<foo>
Foo
</foo>
<!-- init bar() -->
Bar
Foo
<bar>
Bar
</bar>
