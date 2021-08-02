--TEST--
DRCP: oci_pconnect() with oci_close() and oci8.old_oci_close_semantics ON
--EXTENSIONS--
oci8
--INI--
oci8.old_oci_close_semantics=1
oci8.connection_class=test
--FILE--
<?php

require __DIR__."/details.inc";

// Test will open a persistent connection
// Close the connection
// Open another connection
// With oci_close() being a no-op, the same connection will be returned

echo "This is with a OCI_PCONNECT\n";
var_dump($conn1 = oci_pconnect($user,$password,$dbase));
$rn1 = (int)$conn1;
oci_close($conn1);

//  Open another connection

var_dump($conn2 = oci_pconnect($user,$password,$dbase));
$rn2 = (int)$conn2;
oci_close($conn2);

// Compare the resource numbers

if ($rn1 === $rn2)
    echo "Both connections share a resource : OK\n";
else
    echo "Both connections are different : NOT OK\n";

echo "Done\n";

?>
--EXPECTF--
Deprecated: Directive oci8.old_oci_close_semantics is deprecated%s
This is with a OCI_PCONNECT
resource(%d) of type (oci8 persistent connection)
resource(%d) of type (oci8 persistent connection)
Both connections share a resource : OK
Done
