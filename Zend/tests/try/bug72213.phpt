--TEST--
Bug #72213 (Finally leaks on nested exceptions)
--XFAIL--
See https://bugs.php.net/bug.php?id=72213
--FILE--
<?php
function test() {
	try {
		throw new Exception('a');
	} finally {
		try {
			throw new Exception('b');
		} finally {
		}
	}
}

try {
	test();
} catch (Exception $e) {
	var_dump($e->getMessage());
	var_dump($e->getPrevious()->getMessage());
}
?>
--EXPECT--
string(1) "b"
string(1) "a"
