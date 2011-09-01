--TEST--
Bug #51647 (Certificate file without private key (pk in another file) doesn't work)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
require_once("connect.inc");

if ($IS_MYSQLND && !extension_loaded("openssl"))
	die("skip PHP streams lack support for SSL. mysqli is compiled to use mysqlnd which uses PHP streams in turn.");

if (!($link = @my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)))
	die(sprintf("skip Connect failed, [%d] %s", mysqli_connect_errno(), mysqli_connect_error()));

$row = NULL;
if ($res = $link->query('SHOW VARIABLES LIKE "have_ssl"')) {
	$row = $res->fetch_row();
} else {
	if ($link->errno == 1064 && ($res = $link->query("SHOW VARIABLES"))) {
		while ($row = $res->fetch_row())
			if ($row[0] == 'have_ssl')
				break;
	} else {
		die(sprintf("skip Failed to test for MySQL SSL support, [%d] %s", $link->errno, $link->error));
	}
}
	

if (empty($row))
	die(sprintf("skip Failed to test for MySQL SSL support, [%d] %s", $link->errno, $link->error));

if ($row[1] == 'NO')
	die(sprintf("skip MySQL has no SSL support, [%d] %s", $link->errno, $link->error));

$link->close();
?>
--FILE--
<?php
	include ("connect.inc");

	if (!is_object($link = mysqli_init()))
		printf("[001] Cannot create link\n");

	$path_to_pems = !$IS_MYSQLND? "ext/mysqli/tests/" : "";
	if (!$link->ssl_set("{$path_to_pems}client-key.pem", "{$path_to_pems}client-cert.pem", "{$path_to_pems}cacert.pem","",""))
		printf("[002] [%d] %s\n", $link->errno, $link->error);

	if (!my_mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket)) {
		printf("[003] Connect failed, [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	if (!$res = $link->query('SHOW STATUS like "Ssl_cipher"')) {
		if (1064 == $link->errno) {
			/* ERROR 1064 (42000): You have an error in your SQL syntax;  = sql strict mode */
			if ($res = $link->query("SHOW STATUS")) {
				while ($row = $res->fetch_assoc())
					if ($row['Variable_name'] == 'Ssl_cipher')
						break;
			} else {
				printf("[005] [%d] %s\n", $link->errno, $link->error);
			}
		} else {
			printf("[004] [%d] %s\n", $link->errno, $link->error);
		}
	} else {
		if (!$row = $res->fetch_assoc())
			printf("[006] [%d] %s\n", $link->errno, $link->error);
	}

	var_dump($row);

	print "done!";
?>
--EXPECTF--
array(2) {
  ["Variable_name"]=>
  string(10) "Ssl_cipher"
  ["Value"]=>
  string(%d) "%S"
}
done!
