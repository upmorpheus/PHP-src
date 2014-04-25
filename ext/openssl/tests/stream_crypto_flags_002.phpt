--TEST--
TLSv1.1 and TLSv1.2 bitwise stream crypto flag assignment
--SKIPIF--
<?php
if (!extension_loaded("openssl")) die("skip openssl not loaded");
if (!function_exists("proc_open")) die("skip no proc_open");
if (OPENSSL_VERSION_NUMBER < 0x10001001) die("skip OpenSSLv1.0.1 required");
--FILE--
<?php
$serverCode = <<<'CODE'
    $serverUri = "ssl://127.0.0.1:64321";
    $serverFlags = STREAM_SERVER_BIND | STREAM_SERVER_LISTEN;
    $serverCtx = stream_context_create(['ssl' => [
        'local_cert' => __DIR__ . '/bug54992.pem'
    ]]);

    $server = stream_socket_server($serverUri, $errno, $errstr, $serverFlags, $serverCtx);
    phpt_notify();

    @stream_socket_accept($server, 1);
    @stream_socket_accept($server, 1);
    @stream_socket_accept($server, 1);
    @stream_socket_accept($server, 1);
CODE;

$clientCode = <<<'CODE'
    $serverUri = "ssl://127.0.0.1:64321";
    $clientFlags = STREAM_CLIENT_CONNECT;
    $clientCtx = stream_context_create(['ssl' => [
        'verify_peer' => true,
        'cafile' => __DIR__ . '/bug54992-ca.pem',
        'peer_name' => 'bug54992.local',
    ]]);

    phpt_wait();

    stream_context_set_option($clientCtx, 'ssl', 'crypto_method', STREAM_CRYPTO_METHOD_TLSv1_0_CLIENT);
    var_dump(stream_socket_client($serverUri, $errno, $errstr, 2, $clientFlags, $clientCtx));

    stream_context_set_option($clientCtx, 'ssl', 'crypto_method', STREAM_CRYPTO_METHOD_TLSv1_1_CLIENT);
    var_dump(stream_socket_client($serverUri, $errno, $errstr, 2, $clientFlags, $clientCtx));

    stream_context_set_option($clientCtx, 'ssl', 'crypto_method', STREAM_CRYPTO_METHOD_TLSv1_2_CLIENT);
    var_dump(stream_socket_client($serverUri, $errno, $errstr, 2, $clientFlags, $clientCtx));

    stream_context_set_option($clientCtx, 'ssl', 'crypto_method', STREAM_CRYPTO_METHOD_TLS_CLIENT);
    var_dump(stream_socket_client($serverUri, $errno, $errstr, 2, $clientFlags, $clientCtx));
CODE;

include 'ServerClientTestCase.inc';
ServerClientTestCase::getInstance()->run($clientCode, $serverCode);
--EXPECTF--
resource(%d) of type (stream)
resource(%d) of type (stream)
resource(%d) of type (stream)
resource(%d) of type (stream)

