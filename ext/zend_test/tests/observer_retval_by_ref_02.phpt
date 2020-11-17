--TEST--
Observer: Retvals by reference are observable that are: IS_TMP_VAR
--SKIPIF--
<?php if (!extension_loaded('zend-test')) die('skip: zend-test extension required'); ?>
--INI--
zend_test.observer.enabled=1
zend_test.observer.observe_all=1
zend_test.observer.show_return_value=1
--FILE--
<?php
function &foo() {
    $retval = 'I should be ';
    return $retval . 'observable'; // IS_TMP_VAR
}

$res = foo(); // Retval used
foo(); // Retval unused

echo 'Done' . PHP_EOL;
?>
--EXPECTF--
<!-- init '%s/observer_retval_by_ref_%d.php' -->
<file '%s/observer_retval_by_ref_%d.php'>
  <!-- init foo() -->
  <foo>

Notice: Only variable references should be returned by reference in %s on line %d
  </foo:'I should be observable'>
  <foo>

Notice: Only variable references should be returned by reference in %s on line %d
  </foo:'I should be observable'>
Done
</file '%s/observer_retval_by_ref_%d.php'>
