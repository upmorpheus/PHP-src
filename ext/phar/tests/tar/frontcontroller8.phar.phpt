--TEST--
Phar front controller no index file 404 tar-based
--EXTENSIONS--
phar
--ENV--
SCRIPT_NAME=/frontcontroller8.phar.php
REQUEST_URI=/frontcontroller8.phar.php/
PATH_INFO=/
--FILE_EXTERNAL--
files/frontcontroller3.phar.tar
--EXPECTHEADERS--
Status: 404 Not Found
--EXPECT--
<html>
 <head>
  <title>File Not Found</title>
 </head>
 <body>
  <h1>404 - File Not Found</h1>
 </body>
</html>
