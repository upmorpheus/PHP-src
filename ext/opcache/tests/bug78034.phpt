--TEST--
Bug #78034: "pecl" tool fails with abort assertion in zend_ssa.c
--FILE--
<?php

function &ref() {}

class Test {
    function method($bool) {
        if (!$bool) {
            $this->foo = &ref();
        }

        $this->foo = &ref();
    }
}

?>
===DONE===
--EXPECT--
===DONE===
