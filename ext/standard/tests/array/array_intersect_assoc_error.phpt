--TEST--
Test array_intersect_assoc() function : error conditions(Bug#43197)
--FILE--
<?php
echo "*** Testing array_intersect_assoc() : error conditions ***\n";

// Zero arguments
echo "\n-- Testing array_intersect_assoc() function with Zero arguments --\n";
try {
    var_dump( array_intersect_assoc() );
} catch (ArgumentCountError $e) {
    echo $e->getMessage(), "\n";
}

// Testing array_intersect_assoc with one less than the expected number of arguments
echo "\n-- Testing array_intersect_assoc() function with less than expected no. of arguments --\n";
$arr1 = array(1, 2);
try {
    var_dump( array_intersect_assoc($arr1) );
} catch (ArgumentCountError $e) {
    echo $e->getMessage(), "\n";
}

echo "Done";
?>
--EXPECT--
*** Testing array_intersect_assoc() : error conditions ***

-- Testing array_intersect_assoc() function with Zero arguments --
At least 2 parameters are required, 0 given

-- Testing array_intersect_assoc() function with less than expected no. of arguments --
At least 2 parameters are required, 1 given
Done
