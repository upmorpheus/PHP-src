--TEST--
Bug #29844 (SOAP doesn't return the result of a valid SOAP request)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

class hello_world {   
  public function hello($to) {
    return 'Hello ' . $to;
  }    
}

class LocalSoapClient extends SoapClient {

  function LocalSoapClient($wsdl, $options) {
    $this->SoapClient($wsdl, $options);
    $this->server = new SoapServer($wsdl, $options);
    $this->server->setClass('hello_world');;
  }

  function __doRequest($request, $location, $action, $version) {
    ob_start();
    $this->server->handle($request);
    $response = ob_get_contents();
    ob_end_clean();
    return $response;
  }

}

$client = new LocalSoapClient(dirname(__FILE__)."/bug29844.wsdl", array("trace"=>1)); 
var_dump($client->hello('davey'));
?>
--EXPECT--
string(11) "Hello davey"
