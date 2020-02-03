--TEST--
SOAP 1.2: T80 echoOk
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
$HTTP_RAW_POST_DATA = <<<EOF
<?xml version="1.0"?>
<env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope">
  <env:Body>
    <test:echoOk xmlns:test="http://example.org/ts-tests" env:encodingStyle="http://example.org/PoisonEncoding">foo</test:echoOk>
  </env:Body>
</env:Envelope>
EOF;
include "soap12-test.inc";
?>
--EXPECT--
<?xml version="1.0" encoding="UTF-8"?>
<env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope"><env:Body><env:Fault><env:Code><env:Value>env:DataEncodingUnknown</env:Value></env:Code><env:Reason><env:Text>Unknown Data Encoding Style</env:Text></env:Reason></env:Fault></env:Body></env:Envelope>
