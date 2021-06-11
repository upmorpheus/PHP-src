--TEST--
enchant_dict_add_to_session() function
--CREDITS--
marcosptf - <marcosptf@yahoo.com.br>
--EXTENSIONS--
enchant
--SKIPIF--
<?php
if (!is_object(enchant_broker_init())) {die("skip, resource dont load\n");}
if (!is_array(enchant_broker_list_dicts(enchant_broker_init()))) {die("skip, no dictionary installed on this machine! \n");}
?>
--FILE--
<?php
$broker = enchant_broker_init();
$dicts = enchant_broker_list_dicts($broker);
$newWord = "iLoveJavaScript";

if (is_object($broker)) {
    echo("OK\n");
    $requestDict = enchant_broker_request_dict($broker, $dicts[0]['lang_tag']);

    if ($requestDict) {
        echo("OK\n");
        $AddtoSessionDict = enchant_dict_add_to_session($requestDict,$newWord);

        if (NULL === $AddtoSessionDict) {
            var_dump($AddtoSessionDict);
        } else {
            echo("dict add to session failed\n");

        }

    } else {
        echo("broker request dict failed\n");
    }
} else {
    echo("broker is not a resource; failed;\n");

}
echo "OK\n";
?>
--EXPECT--
OK
OK
NULL
OK
