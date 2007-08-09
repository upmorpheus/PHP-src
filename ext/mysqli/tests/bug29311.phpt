--TEST--
constructor test
--SKIPIF--
<?php 
require_once('skipif.inc'); 
require_once('skipifconnectfailure.inc');
?>
--FILE--
<?php
	include "connect.inc";
	
	/* class 1 calls parent constructor */
	class mysql1 extends mysqli {
		function __construct() {
			global $host, $user, $passwd, $db, $port, $socket;
			parent::__construct($host, $user, $passwd, $db, $port, $socket);
		}
	}

	/* class 2 has an own constructor */
	class mysql2 extends mysqli {
		
		function __construct() {
			global $host, $user, $passwd, $db, $port, $socket;
			$this->connect($host, $user, $passwd, $db, $port, $socket);
		}
	}

	/* class 3 has no constructor */
	class mysql3 extends mysqli {
		
	}

	$foo[0] = new mysql1();	
	$foo[1] = new mysql2();	
	$foo[2] = new mysql3($host, $user, $passwd, $db, $port, $socket);


	for ($i=0; $i < 3; $i++) {
		if (($result = $foo[$i]->query("SELECT DATABASE()"))) {
			$row = $result->fetch_row();
			printf("%d: %s\n", $i, $row[0]);
			$result->close();
		}
		$foo[$i]->close();
	}
?>
--EXPECTF--
0: test
1: test
2: test
