--TEST--
ext/sockets - socket_getpeername_ipv6loop - basic test
--CREDITS--
# TestFest 2009 - NorwayUG
# $Id: socket_getpeername_ipv6loop.phpt 494 2009-06-09 20:38:05Z tatjana.andersen@redpill-linpro.com $
--SKIPIF--
<?php   
        if (!extension_loaded('sockets')) {
                die('skip sockets extension not available.');
        }
?>
--FILE--
<?php   
	/* Bind and connect sockets to localhost */
	$localhost = '::1';

	/* Hold the port associated to address */
	$port = 31337;
	
        /* Setup socket server */
        $server = socket_create(AF_INET6, SOCK_STREAM, getprotobyname('tcp'));
        if (!$server) {
                die('Unable to create AF_INET6 socket [server]');
        }
	
        if (!socket_bind($server, $localhost, $port)) {
                die('Unable to bind to '.$localhost.':'.$port);
        }
        if (!socket_listen($server, 2)) {
                die('Unable to listen on socket');
        }

        /* Connect to it */
        $client = socket_create(AF_INET6, SOCK_STREAM, getprotobyname('tcp'));
        if (!$client) {
                die('Unable to create AF_INET6 socket [client]');
        }
        if (!socket_connect($client, $localhost, $port)) {
                die('Unable to connect to server socket');
        }

        /* Accept that connection */
        $socket = socket_accept($server);
        if (!$socket) {
	        die('Unable to accept connection');
        }

	if (!socket_getpeername($client, $address, $port)) {
	   	die('Unable to retrieve peer name');
	}
        var_dump($address, $port);

        socket_close($client);
        socket_close($socket);
        socket_close($server);
?>
--EXPECT--
string(3) "::1"
int(31337)
