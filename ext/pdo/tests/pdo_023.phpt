--TEST--
PDO Common: extending PDO
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) print 'skip';
if (false == getenv('REDIR_TEST_DIR')) print 'skip no driver';
?>
--FILE--
<?php
require getenv('REDIR_TEST_DIR') . 'pdo_test.php';

class PDOStatementX extends PDOStatement
{
    public $test1 = 1;
    
    protected function __construct()
    {
    	$this->test2 = 2;
    	$this->test2 = 22;
    	echo __METHOD__ . "()\n";
    }
    
    function __destruct()
    {
    	echo __METHOD__ . "()\n";
    }
}

class PDODatabaseX extends PDO
{
    public $test1 = 1;
    
    function __destruct()
    {
    	echo __METHOD__ . "()\n";
    }
    
    function test()
    {
    	$this->test2 = 2;
        var_dump($this->test1);
        var_dump($this->test2);
    	$this->test2 = 22;
    }
    
    function query($sql)
    {
    	echo __METHOD__ . "()\n";
    	$stmt = parent::prepare($sql, array(PDO_ATTR_STATEMENT_CLASS=>array('PDOStatementx')));
    	$stmt->execute();
    	return $stmt;
    }
}

$db = PDOTest::factory('PDODatabaseX');
$db->test();
var_dump($db);

$db->query('CREATE TABLE test(id INT NOT NULL PRIMARY KEY, val VARCHAR(10))');
$db->query('INSERT INTO test VALUES(0, \'A\')');
$db->query('INSERT INTO test VALUES(1, \'B\')');


$stmt = $db->query('SELECT val, id FROM test');
var_dump($stmt);
var_dump($stmt->fetchAll(PDO_FETCH_COLUMN|PDO_FETCH_UNIQUE));

$stmt = NULL;
$db = NULL;


?>
--EXPECTF--
int(1)
int(2)
object(PDODatabaseX)#%d (2) {
  ["test1"]=>
  int(1)
  ["test2"]=>
  int(22)
}
PDODatabaseX::query()
PDOStatementX::__construct()
PDOStatementX::__destruct()
PDODatabaseX::query()
PDOStatementX::__construct()
PDOStatementX::__destruct()
PDODatabaseX::query()
PDOStatementX::__construct()
PDOStatementX::__destruct()
PDODatabaseX::query()
PDOStatementX::__construct()
object(PDOStatementX)#%d (3) {
  ["test1"]=>
  int(1)
  ["queryString"]=>
  string(24) "SELECT val, id FROM test"
  ["test2"]=>
  int(22)
}
array(2) {
  ["A"]=>
  string(1) "0"
  ["B"]=>
  string(1) "1"
}
PDOStatementX::__destruct()
PDODatabaseX::__destruct()
