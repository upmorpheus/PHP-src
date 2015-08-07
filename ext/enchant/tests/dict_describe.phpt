--TEST--
enchant_dict_describe() function
--CREDITS--
marcosptf - <marcosptf@yahoo.com.br>
--SKIPIF--
<?php 
if(!extension_loaded('enchant')) die('skip, enchant not loader');
?>
--FILE--
<?php
$broker = enchant_broker_init();
$lang = "en_EN";

if (is_resource($broker)) {
    echo("OK\n");
    $requestDict = enchant_broker_request_dict($broker,$lang);
    
    if ($requestDict) {
        echo("OK\n");
    	$dictDescribe = enchant_dict_describe($requestDict);
    	
        if (is_array($dictDescribe)) {
            echo("OK\n");
            
        } else {
	    echo("broker request dict failed\n");        
        }    	
    } else {
	echo("broker request dict failed\n");        
    }

} else {
    echo("broker is not a resource; failed;\n");
}
?>
--EXPECT--
OK
OK
OK
