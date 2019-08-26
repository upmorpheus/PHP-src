--TEST--
Hash: Attempt to reuse a closed hash context
--FILE--
<?php

$h = hash_init('md5');
hash_final($h);
try { 
    hash_update($h, 'foo');
}
catch (\Error $e) {
    echo $e->getMessage() . "\n";
}

--EXPECT--
hash_update(): supplied resource is not a valid Hash Context resource
