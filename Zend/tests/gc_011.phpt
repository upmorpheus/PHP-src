--TEST--
GC 011: GC and destructors
--INI--
zend.enable_gc=1
--FILE--
<?php
class Foo {
	public $a;
	function __destruct() {
		echo __FUNCTION__,"\n";
	}
}
$a = new Foo();
$a->a = $a;
var_dump($a);
unset($a);
var_dump(gc_collect_cycles());
echo "ok\n"
?>
--EXPECT--
object(Foo)#1 (1) {
  [u"a"]=>
  object(Foo)#1 (1) {
    [u"a"]=>
    *RECURSION*
  }
}
__destruct
int(1)
ok
