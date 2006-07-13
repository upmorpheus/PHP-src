--TEST--
Bug #28969 (Wrong data encoding of special characters)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--INI--
unicode.script_encoding=ISO-8859-1
unicode.output_encoding=ISO-8859-1
--FILE--
<?php
function test() {
  return "��";
//  return utf8_encode("��");
}

class LocalSoapClient extends SoapClient {

  function __construct($wsdl, $options) {
    parent::__construct($wsdl, $options);
    $this->server = new SoapServer($wsdl, $options);
    $this->server->addFunction('test');
  }

  function __doRequest($request, $location, $action, $version) {
    ob_start();
    $this->server->handle($request);
    $response = ob_get_contents();
    ob_end_clean();
    return $response;
  }

}

$x = new LocalSoapClient(NULL,array('location'=>'test://', 
                                    'uri'=>'http://testuri.org',
                                    'encoding'=>'ISO-8859-1')); 
var_dump($x->test());
var_dump("��");
echo "ok\n";
?>
--EXPECT--
string(3) "��"
string(3) "��"
ok
--UEXPECT--
unicode(3) "��"
unicode(3) "��"
ok