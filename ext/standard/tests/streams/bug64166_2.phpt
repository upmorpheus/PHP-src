--TEST--
Bug #64166: quoted-printable-encode stream filter incorrectly discarding whitespace - split reads
--FILE--
<?php

function test_64166($data) {
    $fd = fopen('php://temp', 'w+');
    fwrite($fd, $data);
    rewind($fd);

    $res = stream_filter_append($fd, 'convert.quoted-printable-encode', STREAM_FILTER_READ, array(
        'line-break-chars' => "\n",
        'line-length' => 74
    ));
    $str = "";
    while(($c = fread($fd, 1))!= "") $str .= $c;
    var_dump($str);

    stream_filter_remove($res);

    rewind($fd);
    stream_filter_append($fd, 'convert.quoted-printable-encode', STREAM_FILTER_READ, array(
        'line-break-chars' => "\n",
        'line-length' => 6
    ));
    $str = "";
    while(($c = fread($fd, 1))!= "") $str .= $c;
    var_dump($str);

    fclose($fd);
}

test_64166("FIRST \nSECOND");
test_64166("FIRST  \nSECOND");

?>
--EXPECT--
string(15) "FIRST=20
SECOND"
string(19) "FIRST=
=20
SECON=
D"
string(18) "FIRST=20=20
SECOND"
string(24) "FIRST=
=20=
=20
SECON=
D"
