--TEST--
Bug #36303 (foreach on error_zval produces segfault)
--FILE--
<?php
$x="test";
foreach($x->a->b as &$v) {
}
echo "ok\n";
?>
--EXPECTF--
Warning: Invalid argument supplied for foreach() in %sbug36303.php on line 3
ok
