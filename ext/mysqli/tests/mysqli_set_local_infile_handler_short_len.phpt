--TEST--
mysqli_set_local_infile_handler() - report shorter buffer
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifemb.inc');
require_once('skipifconnectfailure.inc');

if (!function_exists('mysqli_set_local_infile_handler'))
	die("skip - function not available.");

require_once('connect.inc');
if (!$TEST_EXPERIMENTAL)
	die("skip - experimental (= unsupported) feature");

if (!$link = mysqli_connect($host, $user, $passwb, $db, $port, $socket))
	die("skip Cannot connect to MySQL");

if (!$res = mysqli_query($link, 'SHOW VARIABLES LIKE "local_infile"')) {
	mysqli_close($link);
	die("skip Cannot check if Server variable 'local_infile' is set to 'ON'");
}

$row = mysqli_fetch_assoc($res);
mysqli_free_result($res);
mysqli_close($link);

if ('ON' != $row['Value'])
	die(sprintf("skip Server variable 'local_infile' seems not set to 'ON', found '%s'",
		$row['Value']));
?>
--INI--
mysqli.allow_local_infile=1
--FILE--
<?php
	require_once('connect.inc');
	require_once('local_infile_tools.inc');
	require_once('table.inc');

	function callback_short_len($fp, &$buffer, $buflen, &$error) {
		static $invocation = 0;

		printf("Callback: %d\n", $invocation);

		$invocation++;
		if (!is_resource($fp))
			printf("[012] First argument passed to callback is not a resource but %s/%s\n",
				$fp, gettype($fp));

		if (!$buffer = fread($fp, $buflen)) {
			if ($invocation == 1) {
				printf("[013] Cannot read from stream\n");
				$error = 'Cannot read from stream';
			} else {
				return strlen($buffer);
			}
		}

		$lines = explode("\n", $buffer);
		if (count($lines) != 4 && strlen($buffer) > 0) {
			printf("[014] Test is too simple to handle a buffer of size %d that cannot hold all lines\n", $buflen);
			$error = 'Parser too simple';
		}

		$buffer = '';
		foreach ($lines as $k => $line) {
			if ('' === trim($line))
					continue;

			$columns = explode(';', $line);
			if (empty($columns)) {
				printf("[015] Cannot parse columns\n");
				$error = 'Cannot parse columns';
			}

			// increase id column value
			$columns[0] += 1;
			$buffer .= implode(';', $columns);
			$buffer .= "\n";
		}

		/* report the wrong length */
		return strlen($buffer) - 1;
	}

	$file = create_standard_csv(1);
	$expected = array(
		array('id' => 98,   'label' => 'x'),
		array('id' => 99,   'label' => 'y'),
		array('id' => 100,  'label' => 'z'),
	);
	try_handler(20, $link, $file, 'callback_short_len', $expected);

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<?php
	require_once("clean_table.inc");
?>
--EXPECTF--
Callback set to 'callback_short_len'
Callback: 0

Warning: mysqli_query(): Mismatch between the return value of the callback and the content length of the buffer. in %s on line %d
[022] LOAD DATA failed, [2000] Mismatch between the return value of the callback and the content length of the buffer.
[024/0] [0] ''
done!