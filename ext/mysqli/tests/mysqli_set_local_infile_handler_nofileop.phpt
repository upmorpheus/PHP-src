--TEST--
mysqli_set_local_infile_handler() - do not use the file pointer
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
?>
--FILE--
<?php
	require_once('connect.inc');
	require_once('local_infile_tools.inc');
	require_once('table.inc');

	function callback_nofileop($fp, &$buffer, $buflen, &$error) {
		static $invocation = 0;

		printf("Callback: %d\n", $invocation++);
		flush();

		$buffer = "1;'a';\n";
		if ($invocation > 10)
			return 0;

		return strlen($buffer);
	}

	$file = create_standard_csv(1);
	$expected = array(array('id' => 1, 'label' => 'a'));
	try_handler(20, $link, $file, 'callback_nofileop', $expected);

	mysqli_close($link);
	print "done!";
?>
--EXPECTF--
Callback set to 'callback_nofileop'
Callback: 0
Callback: 1
Callback: 2
Callback: 3
Callback: 4
Callback: 5
Callback: 6
Callback: 7
Callback: 8
Callback: 9
Callback: 10
done!