--TEST--
PDO_sqlite: Testing sqliteCreateAggregate()
--SKIPIF--
<?php if (!extension_loaded('pdo_sqlite')) print 'skip not loaded'; ?>
--FILE--
<?php

$db = new pdo('sqlite::memory:');

$db->query('CREATE TABLE IF NOT EXISTS foobar (id INT AUTO INCREMENT, name TEXT)');

$db->query('INSERT INTO foobar VALUES (NULL, "PHP")');
$db->query('INSERT INTO foobar VALUES (NULL, "PHP6")');

$db->sqliteCreateAggregate('testing', function(&$a, $b) { $a .= $b; return $a; }, function(&$v) { return $v; });


foreach ($db->query('SELECT testing(name) FROM foobar') as $row) {
	var_dump($row);
}

$db->query('DROP TABLE foobar');

?>
--EXPECT--
array(2) {
  ["testing(name)"]=>
  string(2) "12"
  [0]=>
  string(2) "12"
}
