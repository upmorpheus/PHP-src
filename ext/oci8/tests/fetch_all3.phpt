--TEST--
oci_fetch_all() - all combinations of flags
--SKIPIF--
<?php if (!extension_loaded('oci8')) die("skip no oci8 extension"); ?>
--FILE--
<?php

require dirname(__FILE__)."/connect.inc";
require dirname(__FILE__).'/create_table.inc';

$insert_sql = "INSERT INTO ".$schema."".$table_name." (id, value) VALUES (:idbv,:vbv)";

$s = oci_parse($c, $insert_sql);
oci_bind_by_name($s, ":idbv", $idbv, SQLT_INT);
oci_bind_by_name($s, ":vbv", $vbv, SQLT_INT);

for ($i = 1; $i <= 4; $i++) {
    $idbv = $i;
    $vbv = -$i;
    oci_execute($s, OCI_DEFAULT);
}

oci_commit($c);

$select_sql = "SELECT ID, VALUE FROM ".$schema."".$table_name." order by id";

$s = oci_parse($c, $select_sql);

echo "None\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1));
var_dump($all);

echo "OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_ASSOC));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_COLUMN\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_COLUMN));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_COLUMN|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_COLUMN|OCI_ASSOC));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM|OCI_ASSOC));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_ASSOC));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_ASSOC));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM|OCI_ASSOC));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_NUM\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_NUM));
var_dump($all);

echo "OCI_FETCHSTATEMENT_BY_ROW|OCI_NUM|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_FETCHSTATEMENT_BY_ROW|OCI_NUM|OCI_ASSOC));
var_dump($all);

echo "OCI_NUM\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_NUM));
var_dump($all);

echo "OCI_NUM|OCI_ASSOC\n";
oci_execute($s);
var_dump(oci_fetch_all($s, $all, 0, -1, OCI_NUM|OCI_ASSOC));
var_dump($all);
require dirname(__FILE__).'/drop_table.inc';
    
echo "Done\n";
?>
--EXPECT--
None
int(4)
array(2) {
  [u"ID"]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [u"VALUE"]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_ASSOC
int(4)
array(2) {
  [u"ID"]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [u"VALUE"]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_COLUMN
int(4)
array(2) {
  [u"ID"]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [u"VALUE"]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_COLUMN|OCI_ASSOC
int(4)
array(2) {
  [u"ID"]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [u"VALUE"]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM
int(4)
array(2) {
  [0]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [1]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM|OCI_ASSOC
int(4)
array(2) {
  [0]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [1]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW
int(4)
array(4) {
  [0]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "1"
    [u"VALUE"]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "2"
    [u"VALUE"]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "3"
    [u"VALUE"]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "4"
    [u"VALUE"]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_ASSOC
int(4)
array(4) {
  [0]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "1"
    [u"VALUE"]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "2"
    [u"VALUE"]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "3"
    [u"VALUE"]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "4"
    [u"VALUE"]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN
int(4)
array(4) {
  [0]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "1"
    [u"VALUE"]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "2"
    [u"VALUE"]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "3"
    [u"VALUE"]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "4"
    [u"VALUE"]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_ASSOC
int(4)
array(4) {
  [0]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "1"
    [u"VALUE"]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "2"
    [u"VALUE"]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "3"
    [u"VALUE"]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [u"ID"]=>
    unicode(1) "4"
    [u"VALUE"]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM
int(4)
array(4) {
  [0]=>
  array(2) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [0]=>
    unicode(1) "2"
    [1]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [0]=>
    unicode(1) "3"
    [1]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [0]=>
    unicode(1) "4"
    [1]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_FETCHSTATEMENT_BY_COLUMN|OCI_NUM|OCI_ASSOC
int(4)
array(4) {
  [0]=>
  array(2) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [0]=>
    unicode(1) "2"
    [1]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [0]=>
    unicode(1) "3"
    [1]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [0]=>
    unicode(1) "4"
    [1]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_NUM
int(4)
array(4) {
  [0]=>
  array(2) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [0]=>
    unicode(1) "2"
    [1]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [0]=>
    unicode(1) "3"
    [1]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [0]=>
    unicode(1) "4"
    [1]=>
    unicode(2) "-4"
  }
}
OCI_FETCHSTATEMENT_BY_ROW|OCI_NUM|OCI_ASSOC
int(4)
array(4) {
  [0]=>
  array(2) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(2) "-1"
  }
  [1]=>
  array(2) {
    [0]=>
    unicode(1) "2"
    [1]=>
    unicode(2) "-2"
  }
  [2]=>
  array(2) {
    [0]=>
    unicode(1) "3"
    [1]=>
    unicode(2) "-3"
  }
  [3]=>
  array(2) {
    [0]=>
    unicode(1) "4"
    [1]=>
    unicode(2) "-4"
  }
}
OCI_NUM
int(4)
array(2) {
  [0]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [1]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
OCI_NUM|OCI_ASSOC
int(4)
array(2) {
  [0]=>
  array(4) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
    [3]=>
    unicode(1) "4"
  }
  [1]=>
  array(4) {
    [0]=>
    unicode(2) "-1"
    [1]=>
    unicode(2) "-2"
    [2]=>
    unicode(2) "-3"
    [3]=>
    unicode(2) "-4"
  }
}
Done
