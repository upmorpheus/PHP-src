--TEST--
Observer: Nested fibers with both unfinished
--EXTENSIONS--
zend_test
--INI--
zend_test.observer.enabled=1
zend_test.observer.fiber_switch=1
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::suspend();

    $fiber = new Fiber(function (): void {
        Fiber::suspend();
    });

    $fiber->start();

    Fiber::suspend();
});

$fiber->start();
$fiber->resume();

?>
--EXPECTF--
<!-- init '%sobserver_fiber_05.php' -->
<!-- switching from fiber %s to %s -->
<init '%s'>
<!-- init {closure}() -->
<!-- switching from fiber %s to %s -->
<suspend '%s'>
<!-- switching from fiber %s to %s -->
<resume '%s'>
<!-- switching from fiber %s to %s -->
<init '%s'>
<!-- init {closure}() -->
<!-- switching from fiber %s to %s -->
<suspend '%s'>
<!-- switching from fiber %s to %s -->
<suspend '%s'>
<!-- switching from fiber %s to %s -->
<destroying '%s'>
<!-- switching from fiber %s to %s -->
<destroying '%s'>
<!-- switching from fiber %s to %s -->
<destroyed '%s'>
<!-- switching from fiber %s to %s -->
<destroyed '%s'>
