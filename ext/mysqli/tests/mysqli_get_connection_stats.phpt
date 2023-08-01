--TEST--
mysqli_get_connection_stats()
--INI--
mysqlnd.collect_statistics="1"
mysqlnd.collect_memory_statistics="1"
--EXTENSIONS--
mysqli
--SKIPIF--
<?PHP
require_once 'skipifconnectfailure.inc';
if (!function_exists('mysqli_get_connection_stats')) {
    die("skip only available with mysqlnd");
}
?>
--FILE--
<?php
    require_once 'connect.inc';
    if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
        printf("Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
            $host, $user, $db, $port, $socket);
        exit(1);
    }

    if (!is_array($info = mysqli_get_connection_stats($link)) || empty($info))
        printf("[003] Expecting array/any_non_empty, got %s/%s\n", gettype($info), $info);

    if (!is_array($info2 = mysqli_get_client_stats()) || empty($info2))
        printf("[004] Expecting array/any_non_empty, got %s/%s\n", gettype($info2), $info2);

    foreach ($info as $k => &$v) {
        if (strpos($k, "mem_") === 0) {
            $v = 0;
        }
    }
    foreach ($info2 as $k => &$v) {
        if (strpos($k, "mem_") === 0) {
            $v = 0;
        }
    }

    if ($info !== $info2) {
        printf("[005] The hashes should be identical except of the memory related fields\n");
        var_dump($info);
        var_dump($info2);
    }

    if (!is_array($info = $link->get_connection_stats()) || empty($info))
        printf("[006] Expecting array/any_non_empty, got %s/%s\n", gettype($info), $info);

    foreach ($info as $k => &$v) {
        if (strpos($k, "mem_") === 0) {
            $v = 0;
        }
    }

    if ($info !== $info2) {
        printf("[007] The hashes should be identical except of the memory related fields\n");
        var_dump($info);
        var_dump($info2);
    }

    mysqli_close($link);
    if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
        printf("Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
            $host, $user, $db, $port, $socket);
        exit(1);
    }

    if (!is_array($info = mysqli_get_connection_stats($link)) || empty($info))
        printf("[008] Expecting array/any_non_empty, got %s/%s\n", gettype($info), $info);

    if (!is_array($info2 = mysqli_get_client_stats()) || empty($info2))
        printf("[009] Expecting array/any_non_empty, got %s/%s\n", gettype($info2), $info2);

    // assuming the test is run in a plain-vanilla CLI environment
    if ($info === $info2) {
        printf("[010] The hashes should not be identical\n");
        var_dump($info);
        var_dump($info2);
    }

    print "done!";
?>
--EXPECT--
done!
