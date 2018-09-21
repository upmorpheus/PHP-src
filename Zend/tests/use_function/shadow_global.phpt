--TEST--
shadowing a global function with a local version
--FILE--
<?php

namespace {
    require 'includes/global_bar.inc';
    require 'includes/foo_bar.inc';
}

namespace {
    var_dump(bar());
}

namespace {
    use function foo\bar;
    var_dump(bar());
    echo "Done\n";
}

?>
--EXPECT--
string(10) "global bar"
string(9) "local bar"
Done
