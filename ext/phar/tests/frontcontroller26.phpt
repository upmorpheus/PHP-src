--TEST--
Phar front controller with unknown extension mime type
--EXTENSIONS--
phar
--ENV--
SCRIPT_NAME=/frontcontroller26.php
REQUEST_URI=/frontcontroller26.php/unknown.ext
PATH_INFO=/unknown.ext
--FILE_EXTERNAL--
files/frontcontroller8.phar
--EXPECTHEADERS--
Content-type: application/octet-stream
--EXPECT--
<?php var_dump("hi");
