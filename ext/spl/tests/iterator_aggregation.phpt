--TEST--
SPL: Iterator aggregating inner iterator's methods
--SKIPIF--
<?php if (!extension_loaded("spl")) print "skip"; ?>
--FILE--
<?php

class NumericArrayIterator implements Iterator
{
	protected $a;
	protected $i = 0;

	public function __construct($a)
	{
		echo __METHOD__ . "\n";
		$this->a = $a;
	}

	public function rewind()
	{
		echo __METHOD__ . "\n";
		$this->i = 0;
	}

	public function valid()
	{
		$ret = $this->i < count($this->a);
		echo __METHOD__ . '(' . ($ret ? 'true' : 'false') . ")\n";
		return $ret;
	}

	public function key()
	{
		echo __METHOD__ . "\n";
		return $this->i;
	}

	public function current()
	{
		echo __METHOD__ . "\n";
		return $this->a[$this->i];
	}

	public function next()
	{
		echo __METHOD__ . "\n";
		$this->i++;
	}
	
	public function greaterThen($comp)
	{
		echo get_class($this) . '::' . __FUNCTION__ . '(' . $comp . ")\n";
		return $this->current() > $comp;
	}
}

class SeekableNumericArrayIterator extends NumericArrayIterator implements SeekableIterator
{
	public function seek($index)
	{
		if ($index < count($this->a)) {
			$this->i = $index;
		}
		echo __METHOD__ . '(' . $index . ")\n";
	}
}

$a = array(1, 2, 3, 4, 5);
$it = new LimitIterator(new NumericArrayIterator($a), 1, 3);
foreach ($it as $v)
{
	print $v . ' is ' . ($it->greaterThen(2) ? 'greater than 2' : 'less than or equal 2') . "\n";
}

echo "===SEEKABLE===\n";
$a = array(1, 2, 3, 4, 5);
$it = new LimitIterator(new SeekableNumericArrayIterator($a), 1, 3);
foreach($it as $v)
{
	print $v . ' is ' . ($it->greaterThen(2) ? 'greater than 2' : 'less than or equal 2') . "\n";
}

echo "===STACKED===\n";
$a = array(1, 2, 3, 4, 5);
$it = new CachingIterator(new LimitIterator(new SeekableNumericArrayIterator($a), 1, 3));
foreach($it as $v)
{
	print $v . ' is ' . ($it->greaterThen(2) ? 'greater than 2' : 'less than or equal 2') . "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
NumericArrayIterator::__construct
NumericArrayIterator::rewind
NumericArrayIterator::valid(true)
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
LimitIterator::greaterThen(2)
2 is less than or equal 2
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
LimitIterator::greaterThen(2)
3 is greater than 2
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
LimitIterator::greaterThen(2)
4 is greater than 2
NumericArrayIterator::next
===SEEKABLE===
NumericArrayIterator::__construct
NumericArrayIterator::rewind
SeekableNumericArrayIterator::seek(1)
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
LimitIterator::greaterThen(2)
2 is less than or equal 2
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
LimitIterator::greaterThen(2)
3 is greater than 2
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
LimitIterator::greaterThen(2)
4 is greater than 2
NumericArrayIterator::next
===STACKED===
NumericArrayIterator::__construct
NumericArrayIterator::rewind
SeekableNumericArrayIterator::seek(1)
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
CachingIterator::greaterThen(2)
2 is less than or equal 2
NumericArrayIterator::next
NumericArrayIterator::valid(true)
NumericArrayIterator::current
NumericArrayIterator::key
CachingIterator::greaterThen(2)
3 is greater than 2
NumericArrayIterator::next
CachingIterator::greaterThen(2)
4 is greater than 2
===DONE===
