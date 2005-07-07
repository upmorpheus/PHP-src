--TEST--
PDO Common: PDO_FETCH_FUNC and statement overloading
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) print 'skip';
$dir = getenv('REDIR_TEST_DIR');
if (false == $dir) print 'skip no driver';
require_once $dir . 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require getenv('REDIR_TEST_DIR') . 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec('CREATE TABLE test(id int NOT NULL PRIMARY KEY, val VARCHAR(10), grp VARCHAR(10))');
$db->exec('INSERT INTO test VALUES(1, \'A\', \'Group1\')'); 
$db->exec('INSERT INTO test VALUES(2, \'B\', \'Group1\')'); 
$db->exec('INSERT INTO test VALUES(3, \'C\', \'Group2\')'); 
$db->exec('INSERT INTO test VALUES(4, \'D\', \'Group2\')'); 

class DerivedStatement extends PDOStatement
{
	private function __construct($name, $db)
	{
		$this->name = $name;
		echo __METHOD__ . "($name)\n";
	}

	function retrieve($id, $val) {
		echo __METHOD__ . "($id,$val)\n";
		return array($id=>$val);
	}
}

$select1 = $db->prepare('SELECT grp, id FROM test');
$select2 = $db->prepare('SELECT id, val FROM test');
$derived = $db->prepare('SELECT id, val FROM test', array(PDO_ATTR_STATEMENT_CLASS=>array('DerivedStatement', array('Overloaded', $db))));

class Test1
{
	public function __construct($id, $val)
	{
		echo __METHOD__ . "($id,$val)\n";
		$this->id = $id;
		$this->val = $val;
	}

	static public function factory($id, $val)
	{
		echo __METHOD__ . "($id,$val)\n";
		return new self($id, $val);
	}
}

function test($id,$val='N/A')
{
	echo __METHOD__ . "($id,$val)\n";
	return array($id=>$val);
}

$f = new Test1(0,0);

$select1->execute();
var_dump($select1->fetchAll(PDO_FETCH_FUNC|PDO_FETCH_GROUP, 'test'));

$select2->execute();
var_dump($select2->fetchAll(PDO_FETCH_FUNC, 'test'));

$select2->execute();
var_dump($select2->fetchAll(PDO_FETCH_FUNC, array('Test1','factory')));

$select2->execute();
var_dump($select2->fetchAll(PDO_FETCH_FUNC, array($f, 'factory')));

var_dump(get_class($derived));
$derived->execute();
var_dump($derived->fetchAll(PDO_FETCH_FUNC, array($derived, 'retrieve')));

?>
--EXPECTF--
DerivedStatement::__construct(Overloaded)
Test1::__construct(0,0)
test(1,N/A)
test(2,N/A)
test(3,N/A)
test(4,N/A)
array(2) {
  ["Group1"]=>
  array(2) {
    [0]=>
    array(1) {
      [1]=>
      string(3) "N/A"
    }
    [1]=>
    array(1) {
      [2]=>
      string(3) "N/A"
    }
  }
  ["Group2"]=>
  array(2) {
    [0]=>
    array(1) {
      [3]=>
      string(3) "N/A"
    }
    [1]=>
    array(1) {
      [4]=>
      string(3) "N/A"
    }
  }
}
test(1,A)
test(2,B)
test(3,C)
test(4,D)
array(4) {
  [0]=>
  array(1) {
    [1]=>
    string(1) "A"
  }
  [1]=>
  array(1) {
    [2]=>
    string(1) "B"
  }
  [2]=>
  array(1) {
    [3]=>
    string(1) "C"
  }
  [3]=>
  array(1) {
    [4]=>
    string(1) "D"
  }
}
Test1::factory(1,A)
Test1::__construct(1,A)
Test1::factory(2,B)
Test1::__construct(2,B)
Test1::factory(3,C)
Test1::__construct(3,C)
Test1::factory(4,D)
Test1::__construct(4,D)
array(4) {
  [0]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "1"
    ["val"]=>
    string(1) "A"
  }
  [1]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "2"
    ["val"]=>
    string(1) "B"
  }
  [2]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "3"
    ["val"]=>
    string(1) "C"
  }
  [3]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "4"
    ["val"]=>
    string(1) "D"
  }
}
Test1::factory(1,A)
Test1::__construct(1,A)
Test1::factory(2,B)
Test1::__construct(2,B)
Test1::factory(3,C)
Test1::__construct(3,C)
Test1::factory(4,D)
Test1::__construct(4,D)
array(4) {
  [0]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "1"
    ["val"]=>
    string(1) "A"
  }
  [1]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "2"
    ["val"]=>
    string(1) "B"
  }
  [2]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "3"
    ["val"]=>
    string(1) "C"
  }
  [3]=>
  object(Test1)#%d (2) {
    ["id"]=>
    string(1) "4"
    ["val"]=>
    string(1) "D"
  }
}
string(16) "DerivedStatement"
DerivedStatement::retrieve(1,A)
DerivedStatement::retrieve(2,B)
DerivedStatement::retrieve(3,C)
DerivedStatement::retrieve(4,D)
array(4) {
  [0]=>
  array(1) {
    [1]=>
    string(1) "A"
  }
  [1]=>
  array(1) {
    [2]=>
    string(1) "B"
  }
  [2]=>
  array(1) {
    [3]=>
    string(1) "C"
  }
  [3]=>
  array(1) {
    [4]=>
    string(1) "D"
  }
}
