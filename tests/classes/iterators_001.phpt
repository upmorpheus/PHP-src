--TEST--
ZE2 iterators and foreach
--SKIPIF--
<?php if (version_compare(zend_version(), '2.0.0-dev', '<')) die('skip ZendEngine 2 needed'); ?>
<?php if (!class_exists('Iterator')) print "skip interface iterator doesn't exist"; ?>
--FILE--
<?php
class c_iter implements Iterator {

	private $obj;
	private $num = 0;

	function __construct($obj) {
		echo __METHOD__ . "\n";
		$this->num = 0;
		$this->obj = $obj;
	}
	function rewind() {
	}
	function hasMore() {
		$more = $this->num < $this->obj->max;
		echo __METHOD__ . ' = ' .($more ? 'true' : 'false') . "\n";
		return $more;
	}
	function current() {
		echo __METHOD__ . "\n";
		return $this->num;
	}
	function next() {
		echo __METHOD__ . "\n";
		$this->num++;
	}
	function key() {
		echo __METHOD__ . "\n";
		switch($this->num) {
			case 0: return "1st";
			case 1: return "2nd";
			case 2: return "3rd";
			default: return "???";
		}
	}
}
	
class c implements IteratorAggregate {

	public $max = 3;

	function getIterator() {
		echo __METHOD__ . "\n";
		return new c_iter($this);
	}
}

echo "===Array===\n";

$a = array(0,1,2);
foreach($a as $v) {
	echo "array:$v\n";
}

echo "===Manual===\n";
$t = new c();
for ($iter = $t->getIterator(); $iter->hasMore(); $iter->next()) {
	echo $iter->current() . "\n";
}

echo "===foreach/std===\n";
foreach($t as $v) {
	echo "object:$v\n";
}

echo "===foreach/rec===\n";
foreach($t as $v) {
	foreach($t as $w) {
		echo "double:$v:$w\n";
	}
}

echo "===foreach/key===\n";
foreach($t as $i => $v) {
	echo "object:$i=>$v\n";
}

print "Done\n";
exit(0);
?>
--EXPECT--
===Array===
array:0
array:1
array:2
===Manual===
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
0
c_iter::next
c_iter::hasMore = true
c_iter::current
1
c_iter::next
c_iter::hasMore = true
c_iter::current
2
c_iter::next
c_iter::hasMore = false
===foreach/std===
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
object:0
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
object:1
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
object:2
c_iter::hasMore = false
===foreach/rec===
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:0:0
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:0:1
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:0:2
c_iter::hasMore = false
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:1:0
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:1:1
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:1:2
c_iter::hasMore = false
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:2:0
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:2:1
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
double:2:2
c_iter::hasMore = false
c_iter::hasMore = false
===foreach/key===
c::getIterator
c_iter::__construct
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
object:1st=>0
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
object:2nd=>1
c_iter::hasMore = true
c_iter::current
c_iter::key
c_iter::next
object:3rd=>2
c_iter::hasMore = false
Done