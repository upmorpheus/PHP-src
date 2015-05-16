--TEST--
non-existent imported functions should not be looked up in the global table
--FILE--
<?php

require 'includes/global_baz.php';

use function foo\bar\baz;
var_dump(baz());

?>
--EXPECTF--
Fatal error: Uncaught exception 'Error' with message 'Call to undefined function foo\bar\baz()' in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
