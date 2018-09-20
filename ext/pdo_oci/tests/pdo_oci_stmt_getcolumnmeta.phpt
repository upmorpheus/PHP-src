--TEST--
PDO_OCI: PDOStatement->getColumnMeta
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo') || !extension_loaded('pdo_oci')) die('skip not loaded');
require(dirname(__FILE__) . '/../../pdo/tests/pdo_test.inc');
PDOTest::skip();
?>
--FILE--
<?php
require(dirname(__FILE__) . '/../../pdo/tests/pdo_test.inc');
$db = PDOTest::factory();
$db->exec(<<<SQL
BEGIN
   EXECUTE IMMEDIATE 'DROP TABLE test';
EXCEPTION
   WHEN OTHERS THEN
      IF SQLCODE != -942 THEN
         RAISE;
      END IF;
END;
SQL
);
$db->exec("CREATE TABLE test(id INT)");

$db->beginTransaction();

try {

	$stmt = $db->prepare('SELECT id FROM test ORDER BY id ASC');

	// execute() has not been called yet
	// NOTE: no warning
	if (false !== ($tmp = $stmt->getColumnMeta(0)))
		printf("[002] Expecting false got %s\n", var_export($tmp, true));

	$stmt->execute();
	// Warning: PDOStatement::getColumnMeta() expects exactly 1 parameter, 0 given in
	if (false !== ($tmp = @$stmt->getColumnMeta()))
		printf("[003] Expecting false got %s\n", var_export($tmp, true));

	// invalid offset
	if (false !== ($tmp = @$stmt->getColumnMeta(-1)))
		printf("[004] Expecting false got %s\n", var_export($tmp, true));

	// Warning: PDOStatement::getColumnMeta() expects parameter 1 to be int, array given in
	if (false !== ($tmp = @$stmt->getColumnMeta(array())))
		printf("[005] Expecting false got %s\n", var_export($tmp, true));

	// Warning: PDOStatement::getColumnMeta() expects exactly 1 parameter, 2 given in
	if (false !== ($tmp = @$stmt->getColumnMeta(1, 1)))
		printf("[006] Expecting false got %s\n", var_export($tmp, true));

	$emulated =  $stmt->getColumnMeta(0);

	printf("Testing native PS...\n");

	$stmt = $db->prepare('SELECT id FROM test ORDER BY id ASC');
	$stmt->execute();
	$native = $stmt->getColumnMeta(0);
	if (count($native) == 0) {
		printf("[008] Meta data seems wrong, %s / %s\n",
			var_export($native, true), var_export($emulated, true));
	}

	// invalid offset
	if (false !== ($tmp = $stmt->getColumnMeta(1)))
		printf("[009] Expecting false because of invalid offset got %s\n", var_export($tmp, true));


	function test_meta(&$db, $offset, $sql_type, $value, $native_type, $pdo_type) {

		$db->exec(<<<SQL
BEGIN
   EXECUTE IMMEDIATE 'DROP TABLE test';
EXCEPTION
   WHEN OTHERS THEN
      IF SQLCODE != -942 THEN
         RAISE;
      END IF;
END;
SQL
);

		$sql = sprintf('CREATE TABLE test(id INT, label %s)', $sql_type);
		if (!($stmt = @$db->prepare($sql)) || (!@$stmt->execute())) {
			// Some engines might not support the data type
			return true;
		}

		if (!$db->exec(sprintf("INSERT INTO test(id, label) VALUES (1, '%s')", $value))) {
			printf("[%03d] + 1] Insert failed, %d - %s\n", $offset,
				$db->errorCode(), var_export($db->errorInfo(), true));
			return false;
		}

		$stmt = $db->prepare('SELECT id, label FROM test');
		$stmt->execute();
		$meta = $stmt->getColumnMeta(1);
		$row = $stmt->fetch(PDO::FETCH_ASSOC);

		if (empty($meta)) {
			printf("[%03d + 2] getColumnMeta() failed, %d - %s\n", $offset,
				$stmt->errorCode(), var_export($stmt->errorInfo(), true));
			return false;
		}

		$elements = array('flags', 'name', 'len', 'precision', 'pdo_type');
		foreach ($elements as $k => $element)
			if (!isset($meta[$element])) {
				printf("[%03d + 3] Element %s missing, %s\n", $offset,
					$element, var_export($meta, true));
				return false;
			}

		if (!is_null($native_type)) {
			if (!isset($meta['native_type'])) {
				printf("[%03d + 5] Element native_type missing, %s\n", $offset,
					var_export($meta, true));
				return false;
			}

			if (!is_array($native_type))
				$native_type = array($native_type);

			$found = false;
			foreach ($native_type as $k => $type) {
				if ($meta['native_type'] == $type) {
					$found = true;
					break;
				}
			}

			if (!$found) {
				printf("[%03d + 6] Expecting native type %s, %s\n", $offset,
					var_export($native_type, true), var_export($meta, true));
				return false;
			}
		}

		if (!is_null($pdo_type) && ($meta['pdo_type'] != $pdo_type)) {
			printf("[%03d + 6] Expecting PDO type %s got %s (%s)\n", $offset,
				$pdo_type, var_export($meta, true), var_export($meta['native_type']));
			return false;
		}

		return true;
	}

	test_meta($db, 10, 'NUMBER'         , 0                    , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 20, 'NUMBER'         , 256                  , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 20, 'NUMBER'         , 256                  , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 20, 'INT'            , 256                  , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 20, 'INTEGER'        , 256                  , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 20, 'NUMBER'         , 256.01               , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 30, 'NUMBER'         , -8388608             , 'NUMBER', PDO::PARAM_STR);

	test_meta($db, 40, 'NUMBER'         , 2147483648           , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 50, 'NUMBER'         , 4294967295           , 'NUMBER', PDO::PARAM_STR);

	test_meta($db, 60, 'DECIMAL'        , 1.01                 , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 60, 'FLOAT'          , 1.01                 , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 70, 'DOUBLE'         , 1.01                 , 'NUMBER', PDO::PARAM_STR);
	test_meta($db, 60, 'BINARY_FLOAT'   , 1.01                 , 'FLOAT', PDO::PARAM_STR);
	test_meta($db, 70, 'BINARY_DOUBLE'  , 1.01                 , 'DOUBLE', PDO::PARAM_STR);

	test_meta($db, 80, 'DATE'           , '2008-04-23'         , 'DATE', PDO::PARAM_STR);
	test_meta($db, 90, 'TIME'           , '14:37:00'           , 'TIME', PDO::PARAM_STR);
	test_meta($db, 110, 'YEAR'          , '2008'               , 'YEAR', PDO::PARAM_STR);

	test_meta($db, 120, 'CHAR(1)'       , 'a'                  , 'CHAR', PDO::PARAM_STR);
	test_meta($db, 130, 'CHAR(10)'      , '0123456789'         , 'CHAR', PDO::PARAM_STR);
	test_meta($db, 140, 'CHAR(255)'     , str_repeat('z', 255) , 'CHAR', PDO::PARAM_STR);
	test_meta($db, 150, 'VARCHAR(1)'    , 'a'                  , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 160, 'VARCHAR(10)'   , '0123456789'         , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 170, 'VARCHAR(255)'  , str_repeat('z', 255) , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 180, 'VARCHAR2(1)'   , 'a'                  , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 190, 'VARCHAR2(10)'  , '0123456789'         , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 200, 'VARCHAR2(255)' , str_repeat('z', 255) , 'VARCHAR2', PDO::PARAM_STR);

	test_meta($db, 210, 'NCHAR(1)'      , 'a'                  , 'CHAR', PDO::PARAM_STR);
	test_meta($db, 220, 'NCHAR(10)'     , '0123456789'         , 'CHAR', PDO::PARAM_STR);
	test_meta($db, 230, 'NCHAR(255)'    , str_repeat('z', 255) , 'CHAR', PDO::PARAM_STR);
	test_meta($db, 240, 'NVARCHAR2(1)'  , 'a'                  , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 250, 'NVARCHAR2(10)' , '0123456789'         , 'VARCHAR2', PDO::PARAM_STR);
	test_meta($db, 260, 'NVARCHAR2(255)', str_repeat('z', 255) , 'VARCHAR2', PDO::PARAM_STR);

	test_meta($db, 270, 'CLOB'          , str_repeat('b', 255) , 'CLOB', PDO::PARAM_LOB);
	test_meta($db, 280, 'BLOB'          , str_repeat('b', 256) , 'BLOB', PDO::PARAM_LOB);
	test_meta($db, 290, 'NCLOB'         , str_repeat('b', 255) , 'CLOB', PDO::PARAM_LOB);

	test_meta($db, 300, 'LONG'          , str_repeat('b', 256) , 'LONG', PDO::PARAM_STR);
	test_meta($db, 310, 'LONG RAW'      , str_repeat('b', 256) , 'LONG RAW', PDO::PARAM_STR);
	test_meta($db, 320, 'RAW'           , str_repeat('b', 256) , 'RAW', PDO::PARAM_STR);

	$db->exec(<<<SQL
BEGIN
   EXECUTE IMMEDIATE 'DROP TABLE test';
EXCEPTION
   WHEN OTHERS THEN
      IF SQLCODE != -942 THEN
         RAISE;
      END IF;
END;
SQL
);
	$sql = sprintf('CREATE TABLE test(id INT NOT NULL, label INT NULL)');
	if (($stmt = $db->prepare($sql)) && $stmt->execute()) {
		$db->exec('INSERT INTO test(id, label) VALUES (1, 1)');
		$stmt = $db->query('SELECT id, label FROM test');
		$meta = $stmt->getColumnMeta(0);
		if (!isset($meta['flags'])) {
			printf("[1002] No flags contained in metadata %s\n", var_export($meta, true));
		} else {
			$flags = $meta['flags'];
			$found = false;
			foreach ($flags as $k => $flag) {
				if ($flag == 'not_null')
					$found = true;
				if ($flag == 'nullable')
					printf("[1003] Flags seem wrong %s\n", var_export($meta, true));
			}
			if (!$found)
				printf("[1003] Flags seem wrong %s\n", var_export($meta, true));
		}
		$meta = $stmt->getColumnMeta(1);
		if (!isset($meta['flags'])) {
			printf("[1002] No flags contained in metadata %s\n", var_export($meta, true));
		} else {
			$flags = $meta['flags'];
			$found = false;
			foreach ($flags as $k => $flag) {
				if ($flag == 'not_null')
					printf("[1003] Flags seem wrong %s\n", var_export($meta, true));
				if ($flag == 'nullable')
					$found = true;
			}
			if (!$found)
				printf("[1003] Flags seem wrong %s\n", var_export($meta, true));
		}
	}

} catch (PDOException $e) {
	// we should never get here, we use warnings, but never trust a system...
	printf("[001] %s, [%s} %s\n",
		$e->getMessage(), $db->errorInfo(), implode(' ', $db->errorInfo()));
}

$db->exec(<<<SQL
BEGIN
   EXECUTE IMMEDIATE 'DROP TABLE test';
EXCEPTION
   WHEN OTHERS THEN
      IF SQLCODE != -942 THEN
         RAISE;
      END IF;
END;
SQL
);
print "done!";
?>
--EXPECT--
Testing native PS...
done!
