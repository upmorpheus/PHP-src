--TEST--
filter_var() and FILTER_VALIDATE_URL
--FILE--
<?php

$values = Array(
'http://example.com/index.html',	
'http://www.example.com/index.php',	
'http://www.example/img/test.png',	
'http://www.example/img/dir/',	
'http://www.example/img/dir',	
'http//www.example/wrong/url/',	
'http:/www.example',	
'file:///tmp/test.c',	
'ftp://ftp.example.com/tmp/',	
'/tmp/test.c',	
'/',	
'http://',	
'http:/',	
'http:',	
'http',	
'',	
-1,	
array(),	
);
foreach ($values as $value) {
	var_dump(filter_var($value, FILTER_VALIDATE_URL));
}


var_dump(filter_var("qwe", FILTER_VALIDATE_URL, FILTER_FLAG_SCHEME_REQUIRED));
var_dump(filter_var("http://qwe", FILTER_VALIDATE_URL, FILTER_FLAG_SCHEME_REQUIRED));
var_dump(filter_var("http://", FILTER_VALIDATE_URL, FILTER_FLAG_HOST_REQUIRED));
var_dump(filter_var("/tmp/test", FILTER_VALIDATE_URL, FILTER_FLAG_HOST_REQUIRED));
var_dump(filter_var("http://www.example.com", FILTER_VALIDATE_URL, FILTER_FLAG_HOST_REQUIRED));
var_dump(filter_var("http://www.example.com", FILTER_VALIDATE_URL, FILTER_FLAG_PATH_REQUIRED));
var_dump(filter_var("http://www.example.com/path/at/the/server/", FILTER_VALIDATE_URL, FILTER_FLAG_PATH_REQUIRED));
var_dump(filter_var("http://www.example.com/index.html", FILTER_VALIDATE_URL, FILTER_FLAG_QUERY_REQUIRED));
var_dump(filter_var("http://www.example.com/index.php?a=b&c=d", FILTER_VALIDATE_URL, FILTER_FLAG_QUERY_REQUIRED));

echo "Done\n";
?>
--EXPECT--	
string(29) "http://example.com/index.html"
string(32) "http://www.example.com/index.php"
string(31) "http://www.example/img/test.png"
string(27) "http://www.example/img/dir/"
string(26) "http://www.example/img/dir"
string(28) "http//www.example/wrong/url/"
string(17) "http:/www.example"
string(18) "file:///tmp/test.c"
string(26) "ftp://ftp.example.com/tmp/"
string(11) "/tmp/test.c"
string(1) "/"
bool(false)
string(6) "http:/"
string(5) "http:"
string(4) "http"
string(0) ""
string(2) "-1"
bool(false)
bool(false)
string(10) "http://qwe"
bool(false)
bool(false)
string(22) "http://www.example.com"
bool(false)
string(42) "http://www.example.com/path/at/the/server/"
bool(false)
string(40) "http://www.example.com/index.php?a=b&c=d"
Done
