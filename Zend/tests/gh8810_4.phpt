--TEST--
GH-8810: Fix reported line number of multi-line dynamic call
--FILE--
<?php

function foo() {
    throw new Exception();
}

'foo'
    ();

?>
--EXPECTF--
Fatal error: Uncaught Exception in %s:4
Stack trace:
#0 %s(8): foo()
#1 {main}
  thrown in %s on line 4
