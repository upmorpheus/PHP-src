--TEST--
public static array PDO::getAvailableDrivers ( void );
array pdo_drivers ( void );
--CREDITS--
marcosptf - <marcosptf@yahoo.com.br> - #phparty7 - @phpsp - novatec/2015 - sao paulo - br
--FILE--
<?php
print((is_array(PDO::getAvailableDrivers())) ? ("yes") : ("Test 'array stream_get_wrappers ( void );' has failed"));
?>
--EXPECT--
yes
