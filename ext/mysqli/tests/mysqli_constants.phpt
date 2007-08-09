--TEST--
Constants exported by ext/mysqli
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifemb.inc');
require_once('skipifconnectfailure.inc');
?>
--FILE--
<?php
	require("connect.inc");
	require("table.inc");

	$php_version = (int)str_replace('.', '', PHP_VERSION);
	$constants = get_defined_constants(true);
	sort($constants);

	$expected_constants = array(
		'MYSQLI_READ_DEFAULT_GROUP'			=> true,
		'MYSQLI_READ_DEFAULT_FILE'			=> true,
		'MYSQLI_OPT_CONNECT_TIMEOUT'		=> true,
		'MYSQLI_OPT_LOCAL_INFILE'			=> true,
		'MYSQLI_INIT_COMMAND'				=> true,
		'MYSQLI_CLIENT_SSL'					=> true,
		"MYSQLI_CLIENT_COMPRESS"			=> true,
		"MYSQLI_CLIENT_INTERACTIVE"			=> true,
		"MYSQLI_CLIENT_IGNORE_SPACE"		=> true,
		"MYSQLI_CLIENT_NO_SCHEMA"			=> true,
		"MYSQLI_CLIENT_FOUND_ROWS"			=> true,
		"MYSQLI_STORE_RESULT"				=> true,
		"MYSQLI_USE_RESULT"					=> true,
		"MYSQLI_ASSOC"						=> true,
		"MYSQLI_NUM"						=> true,
		"MYSQLI_BOTH"						=> true,
		"MYSQLI_STMT_ATTR_UPDATE_MAX_LENGTH"	=> true,
		"MYSQLI_NOT_NULL_FLAG"				=> true,
		"MYSQLI_PRI_KEY_FLAG"				=> true,
		"MYSQLI_UNIQUE_KEY_FLAG"			=> true,
		"MYSQLI_MULTIPLE_KEY_FLAG"			=> true,
		"MYSQLI_BLOB_FLAG"					=> true,
		"MYSQLI_UNSIGNED_FLAG"				=> true,
		"MYSQLI_ZEROFILL_FLAG"				=> true,
		"MYSQLI_AUTO_INCREMENT_FLAG"		=> true,
		"MYSQLI_TIMESTAMP_FLAG"				=> true,
		"MYSQLI_SET_FLAG"					=> true,
		"MYSQLI_NUM_FLAG"					=> true,
		"MYSQLI_PART_KEY_FLAG"				=> true,
		"MYSQLI_GROUP_FLAG"					=> true,
		"MYSQLI_TYPE_DECIMAL"				=> true,
		"MYSQLI_TYPE_TINY"					=> true,
		"MYSQLI_TYPE_SHORT"					=> true,
		"MYSQLI_TYPE_LONG"					=> true,
		"MYSQLI_TYPE_FLOAT"					=> true,
		"MYSQLI_TYPE_DOUBLE"				=> true,
		"MYSQLI_TYPE_NULL"					=> true,
		"MYSQLI_TYPE_TIMESTAMP"				=> true,
		"MYSQLI_TYPE_LONGLONG"				=> true,
		"MYSQLI_TYPE_INT24"					=> true,
		"MYSQLI_TYPE_DATE"					=> true,
		"MYSQLI_TYPE_TIME"					=> true,
		"MYSQLI_TYPE_DATETIME"				=> true,
		"MYSQLI_TYPE_YEAR"					=> true,
		"MYSQLI_TYPE_NEWDATE"				=> true,
		"MYSQLI_TYPE_ENUM"					=> true,
		"MYSQLI_TYPE_SET"					=> true,
		"MYSQLI_TYPE_TINY_BLOB"				=> true,
		"MYSQLI_TYPE_MEDIUM_BLOB"			=> true,
		"MYSQLI_TYPE_LONG_BLOB"				=> true,
		"MYSQLI_TYPE_BLOB"					=> true,
		"MYSQLI_TYPE_VAR_STRING"			=> true,
		"MYSQLI_TYPE_STRING"				=> true,
		"MYSQLI_TYPE_CHAR"					=> true,
		"MYSQLI_TYPE_INTERVAL"				=> true,
		"MYSQLI_TYPE_GEOMETRY"				=> true,
		"MYSQLI_NO_DATA"					=> true,
		"MYSQLI_REPORT_INDEX"				=> true,
		"MYSQLI_REPORT_STRICT"				=> true,
		"MYSQLI_REPORT_ALL"					=> true,
		"MYSQLI_REPORT_ERROR"				=> true,
		"MYSQLI_REPORT_OFF"					=> true,
		"MYSQLI_SET_CHARSET_NAME"			=> true,
	);

	/* depends on the build - experimental */
	if ($IS_MYSQLND && defined('MYSQLI_OPT_INT_AND_YEARS_AS_INT'))
		$expected_constants['MYSQLI_OPT_INT_AND_YEARS_AS_INT'] = true;

	if ($IS_MYSQLND) {
		$version = 50007 + 1;
	} else {
		$version = mysqli_get_client_version();
	}

	if ($version > 50002) {
		$expected_constants = array_merge($expected_constants, array(
			"MYSQLI_TYPE_NEWDECIMAL"	=> true,
			"MYSQLI_TYPE_BIT"			=> true,
		));
	}

	if ($version > 50003) {
		$expected_constants = array_merge($expected_constants, array(
			"MYSQLI_STMT_ATTR_CURSOR_TYPE"		=> true,
			"MYSQLI_CURSOR_TYPE_NO_CURSOR"		=> true,
			"MYSQLI_CURSOR_TYPE_READ_ONLY"		=> true,
			"MYSQLI_CURSOR_TYPE_FOR_UPDATE"		=> true,
			"MYSQLI_CURSOR_TYPE_SCROLLABLE"		=> true,
		));
	}

	if ($version > 50007) {
		$expected_constants = array_merge($expected_constants, array(
			"MYSQLI_STMT_ATTR_PREFETCH_ROWS"	=> true,
		));
	}

	/* pretty dump test, but that is the best way to mimic mysql.c */
	if (defined('MYSQLI_DATA_TRUNCATED'))
		$expected_constants["MYSQLI_DATA_TRUNCATED"] = true;

	if ($IS_MYSQLND && $php_version >= 600) {
		/* mysqlnd only */
		$expected_constants["MYSQLI_OPT_NUMERIC_AND_DATETIME_AS_UNICODE"] = true;
	} else if (!$IS_MYSQLND) {
		/* libmysql only */

		/* are they available in all versions of ext/mysqli ? */
		$expected_constants["MYSQLI_RPL_MASTER"]	= true;
		$expected_constants["MYSQLI_RPL_SLAVE"]		= true;
		$expected_constants["MYSQLI_RPL_ADMIN"]		= true;
	}


	$unexpected_constants = array();

	foreach ($constants as $group => $consts) {
		foreach ($consts as $name => $value) {
			if (stristr($name, 'mysqli')) {
				$name = strtoupper($name);
				if (isset($expected_constants[$name])) {
					unset($expected_constants[$name]);
				} else {
					$unexpected_constants[$name] = $name;
				}
			}
		}
	}

	if (!empty($unexpected_constants)) {
		printf("Dumping list of unexpected constants\n");
		var_dump($unexpected_constants);
	}

	if (!empty($expected_constants)) {
		printf("Dumping list of missing constants\n");
		var_dump($expected_constants);
	}

	print "done!";
?>
--EXPECTF--
done!