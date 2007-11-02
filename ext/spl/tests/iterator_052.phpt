--TEST--
SPL: RegexIterator::ALL_MATCHES
--SKIPIF--
<?php if (!extension_loaded("spl")) print "skip"; ?>
--FILE--
<?php

class MyRegexIterator extends RegexIterator
{
	public $uk, $re;
	
	function __construct($it, $re, $mode, $flags = 0)
	{
		$this->uk = $flags & self::USE_KEY;
		$this->re = $re;
		parent::__construct($it, $re, $mode, $flags);
	}

	function show()
	{
		foreach($this as $k => $v)
		{
			var_dump($k);
			var_dump($v);
		}
	}
	
	function accept()
	{
		@preg_match_all($this->re, (string)($this->uk ? $this->key() : $this->current()), $sub);
		$ret = parent::accept();
		var_dump($sub == $this->current());
		return $ret;
	}
}

$ar = new ArrayIterator(array('1','1,2','1,2,3','',NULL,array(),'FooBar',',',',,'));
$it = new MyRegexIterator($ar, '/(\d),(\d)/', RegexIterator::ALL_MATCHES);
$it->show();

$it = new MyRegexIterator($ar, '/(\d)/', RegexIterator::ALL_MATCHES);
$it->show();

var_dump($ar);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
bool(true)
int(0)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(1)
array(3) {
  [0]=>
  array(1) {
    [0]=>
    string(3) "1,2"
  }
  [1]=>
  array(1) {
    [0]=>
    string(1) "1"
  }
  [2]=>
  array(1) {
    [0]=>
    string(1) "2"
  }
}
bool(true)
int(2)
array(3) {
  [0]=>
  array(1) {
    [0]=>
    string(3) "1,2"
  }
  [1]=>
  array(1) {
    [0]=>
    string(1) "1"
  }
  [2]=>
  array(1) {
    [0]=>
    string(1) "2"
  }
}
bool(true)
int(3)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(4)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(5)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(6)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(7)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(8)
array(3) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
  [2]=>
  array(0) {
  }
}
bool(true)
int(0)
array(2) {
  [0]=>
  array(1) {
    [0]=>
    string(1) "1"
  }
  [1]=>
  array(1) {
    [0]=>
    string(1) "1"
  }
}
bool(true)
int(1)
array(2) {
  [0]=>
  array(2) {
    [0]=>
    string(1) "1"
    [1]=>
    string(1) "2"
  }
  [1]=>
  array(2) {
    [0]=>
    string(1) "1"
    [1]=>
    string(1) "2"
  }
}
bool(true)
int(2)
array(2) {
  [0]=>
  array(3) {
    [0]=>
    string(1) "1"
    [1]=>
    string(1) "2"
    [2]=>
    string(1) "3"
  }
  [1]=>
  array(3) {
    [0]=>
    string(1) "1"
    [1]=>
    string(1) "2"
    [2]=>
    string(1) "3"
  }
}
bool(true)
int(3)
array(2) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
}
bool(true)
int(4)
array(2) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
}
bool(true)
int(5)
array(2) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
}
bool(true)
int(6)
array(2) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
}
bool(true)
int(7)
array(2) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
}
bool(true)
int(8)
array(2) {
  [0]=>
  array(0) {
  }
  [1]=>
  array(0) {
  }
}
object(ArrayIterator)#%d (1) {
  ["storage":"ArrayIterator":private]=>
  array(9) {
    [0]=>
    %s(1) "1"
    [1]=>
    %s(3) "1,2"
    [2]=>
    %s(5) "1,2,3"
    [3]=>
    %s(0) ""
    [4]=>
    NULL
    [5]=>
    array(0) {
    }
    [6]=>
    %s(6) "FooBar"
    [7]=>
    %s(1) ","
    [8]=>
    %s(2) ",,"
  }
}
===DONE===
