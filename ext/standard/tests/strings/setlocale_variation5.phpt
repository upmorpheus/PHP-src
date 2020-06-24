--TEST--
Test setlocale() function : usage variations - Setting system locale as empty string
--SKIPIF--
<?php
if (substr(PHP_OS, 0, 3) == 'WIN') {
    die('skip Not valid for windows');
}
if (setlocale(LC_ALL,'en_AU.utf8') === false || setlocale(LC_ALL,'en_US.utf8') === false) {
    die('skip en_AU.utf8/en_US.utf8 locales not available');
}
?>
--ENV--
LC_ALL=en_US.utf8;
--FILE--
<?php
/* If locale is empty string "", the locale names will be set from the values of environment variables with the same names as from ENV */

echo "*** Testing setlocale() : usage variations - setting system locale = \"\" ***\n";

//initially setting the locale
setlocale(LC_ALL,'en_AU.utf8');

echo "Locale info, before setting the locale\n";

//returns current locale,before executing setlocale() .
$locale_info_before = localeconv();

var_dump($locale_info_before);

//Testing setlocale()  by giving locale = null
echo "Setting system locale, category = LC_ALL and locale = \"\"\n";
setlocale(LC_ALL, "");

echo "Locale info, after setting the locale\n";

//Returns Current locale,after executing setlocale().
$locale_info_after = localeconv();

var_dump($locale_info_after);

echo "Checking new locale in the system, Expected : the locale names will be set from the values of environment variables\n";
echo "Test ";
if($locale_info_before != $locale_info_after){
  echo "PASSED.";
} else {
  echo "FAILED.";
}

echo "\nDone\n";
?>
--EXPECT--
*** Testing setlocale() : usage variations - setting system locale = "" ***
Locale info, before setting the locale
array(18) {
  ["decimal_point"]=>
  string(1) "."
  ["thousands_sep"]=>
  string(1) ","
  ["int_curr_symbol"]=>
  string(4) "AUD "
  ["currency_symbol"]=>
  string(1) "$"
  ["mon_decimal_point"]=>
  string(1) "."
  ["mon_thousands_sep"]=>
  string(1) ","
  ["positive_sign"]=>
  string(0) ""
  ["negative_sign"]=>
  string(1) "-"
  ["int_frac_digits"]=>
  int(2)
  ["frac_digits"]=>
  int(2)
  ["p_cs_precedes"]=>
  int(1)
  ["p_sep_by_space"]=>
  int(0)
  ["n_cs_precedes"]=>
  int(1)
  ["n_sep_by_space"]=>
  int(0)
  ["p_sign_posn"]=>
  int(1)
  ["n_sign_posn"]=>
  int(1)
  ["grouping"]=>
  array(2) {
    [0]=>
    int(3)
    [1]=>
    int(3)
  }
  ["mon_grouping"]=>
  array(2) {
    [0]=>
    int(3)
    [1]=>
    int(3)
  }
}
Setting system locale, category = LC_ALL and locale = ""
Locale info, after setting the locale
array(18) {
  ["decimal_point"]=>
  string(1) "."
  ["thousands_sep"]=>
  string(1) ","
  ["int_curr_symbol"]=>
  string(4) "USD "
  ["currency_symbol"]=>
  string(1) "$"
  ["mon_decimal_point"]=>
  string(1) "."
  ["mon_thousands_sep"]=>
  string(1) ","
  ["positive_sign"]=>
  string(0) ""
  ["negative_sign"]=>
  string(1) "-"
  ["int_frac_digits"]=>
  int(2)
  ["frac_digits"]=>
  int(2)
  ["p_cs_precedes"]=>
  int(1)
  ["p_sep_by_space"]=>
  int(0)
  ["n_cs_precedes"]=>
  int(1)
  ["n_sep_by_space"]=>
  int(0)
  ["p_sign_posn"]=>
  int(1)
  ["n_sign_posn"]=>
  int(1)
  ["grouping"]=>
  array(2) {
    [0]=>
    int(3)
    [1]=>
    int(3)
  }
  ["mon_grouping"]=>
  array(2) {
    [0]=>
    int(3)
    [1]=>
    int(3)
  }
}
Checking new locale in the system, Expected : the locale names will be set from the values of environment variables
Test PASSED.
Done
