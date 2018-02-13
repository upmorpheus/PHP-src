--TEST--
Testing declare statement with ticks
--FILE--
<?php
register_tick_function(function () { echo "tick\n"; });
function foo() { }

declare(ticks=1) {

$statement;
foo();

}
?>
--EXPECTF--
tick
tick
