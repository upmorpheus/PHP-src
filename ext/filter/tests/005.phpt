--TEST--
GET/REQUEST Test with fifa example data
--INI--
filter.default=notags
--POST--
--GET--
id=f03_photos&pgurl=http%3A//fifaworldcup.yahoo.com/03/en/photozone/index.html
--FILE--
<?php 
echo $_GET['id'];
echo "\n";
echo $_GET['pgurl']; 
echo "\n";
echo $_REQUEST['id'];
echo "\n";
echo $_REQUEST['pgurl']; 
?>
--EXPECT--
f03_photos
http://fifaworldcup.yahoo.com/03/en/photozone/index.html
f03_photos
http://fifaworldcup.yahoo.com/03/en/photozone/index.html
