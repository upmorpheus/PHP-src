--TEST--
SimpleXML: Adding an elements without text
--SKIPIF--
<?php if (!extension_loaded("simplexml")) print "skip"; ?>
--FILE--
<?php 
$xml =b<<<EOF
<people></people>
EOF;

function traverse_xml($xml, $pad = '')
{
  $name = $xml->getName();
  echo "$pad<$name";
  foreach($xml->attributes() as $attr => $value)
  {
    echo " $attr=\"$value\"";
  }
  echo ">" . trim($xml) . "\n";
  foreach($xml->children() as $node)
  {
    traverse_xml($node, $pad.'  ');
  }
  echo $pad."</$name>\n";
}


$people = simplexml_load_string($xml);
traverse_xml($people);
$people->person['name'] = 'John';
traverse_xml($people);

?>
===DONE===
--EXPECTF--
<people>
</people>
<people>
  <person name="John">
  </person>
</people>
===DONE===
