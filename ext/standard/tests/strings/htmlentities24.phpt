--TEST--
Test htmlentities() function
--FILE--
<?php
/* Prototype: string htmlentities ( string $string [, int $quote_style [, string $charset]] );
   Description: Convert all applicable characters to HTML entities
*/

/* retrieving htmlentities from the ANSI character table */
echo "*** Retrieving htmlentities for 256 characters ***\n";
for($i=0; $i<256; $i++)
  var_dump( bin2hex( htmlentities("chr($i)")) );

/* giving arguments as NULL */
echo "\n*** Testing htmlentities() with NULL as first,second and third argument ***\n";
var_dump( htmlentities("\x82\x86\x99\x9f\x80\x82\x81", NULL, 'cp1252') );
var_dump( htmlentities("\x82\x86\x99\x9f\x80\x82\x81", ENT_QUOTES, NULL) ); /* UTF-8 assumed */
var_dump( htmlentities("\x82\x86\x99\x9f\x80\x82\x81", ENT_NOQUOTES, NULL) ); /* UTF-8 assumed */
var_dump( htmlentities("\x82\x86\x99\x9f\x80\x82\x81", ENT_COMPAT, NULL) ); /* UTF-8 assumed */
var_dump( htmlentities(NULL, NULL, NULL) );

/* giving long string to check for proper memory re-allocation */
echo "\n*** Checking for proper memory allocation with long string ***\n";
var_dump( htmlentities("\x82\x86\x99\x9f\x80\x82\x86\x84\x80\x89\x85\x83\x86\x84\x80\x91\x83\x91\x86\x87\x85\x86\x88\x82\x89\x92\x91\x83", ENT_QUOTES, 'cp1252'));

/* giving a normal string */
echo "\n*** Testing a normal string with htmlentities() ***\n";
var_dump( htmlentities("<html> This is a test! </html>") );

/* checking behavior of quote */
echo "\n*** Testing htmlentites() on a quote ***\n";
$str = "A 'quote' is <b>bold</b>";
var_dump( htmlentities($str) );
var_dump( htmlentities($str, ENT_QUOTES) );
var_dump( htmlentities($str, ENT_NOQUOTES) );
var_dump( htmlentities($str, ENT_COMPAT) );

echo "\n*** Testing error conditions ***\n";
/* zero argument */
var_dump( htmlentities() );
/* arguments more than expected */
var_dump( htmlentities("\x84\x91",ENT_QUOTES, 'cp1252', "test1") );

echo "Done\n";
?>
--EXPECTF--
*** Retrieving htmlentities for 256 characters ***
string(12) "636872283029"
string(12) "636872283129"
string(12) "636872283229"
string(12) "636872283329"
string(12) "636872283429"
string(12) "636872283529"
string(12) "636872283629"
string(12) "636872283729"
string(12) "636872283829"
string(12) "636872283929"
string(14) "63687228313029"
string(14) "63687228313129"
string(14) "63687228313229"
string(14) "63687228313329"
string(14) "63687228313429"
string(14) "63687228313529"
string(14) "63687228313629"
string(14) "63687228313729"
string(14) "63687228313829"
string(14) "63687228313929"
string(14) "63687228323029"
string(14) "63687228323129"
string(14) "63687228323229"
string(14) "63687228323329"
string(14) "63687228323429"
string(14) "63687228323529"
string(14) "63687228323629"
string(14) "63687228323729"
string(14) "63687228323829"
string(14) "63687228323929"
string(14) "63687228333029"
string(14) "63687228333129"
string(14) "63687228333229"
string(14) "63687228333329"
string(14) "63687228333429"
string(14) "63687228333529"
string(14) "63687228333629"
string(14) "63687228333729"
string(14) "63687228333829"
string(14) "63687228333929"
string(14) "63687228343029"
string(14) "63687228343129"
string(14) "63687228343229"
string(14) "63687228343329"
string(14) "63687228343429"
string(14) "63687228343529"
string(14) "63687228343629"
string(14) "63687228343729"
string(14) "63687228343829"
string(14) "63687228343929"
string(14) "63687228353029"
string(14) "63687228353129"
string(14) "63687228353229"
string(14) "63687228353329"
string(14) "63687228353429"
string(14) "63687228353529"
string(14) "63687228353629"
string(14) "63687228353729"
string(14) "63687228353829"
string(14) "63687228353929"
string(14) "63687228363029"
string(14) "63687228363129"
string(14) "63687228363229"
string(14) "63687228363329"
string(14) "63687228363429"
string(14) "63687228363529"
string(14) "63687228363629"
string(14) "63687228363729"
string(14) "63687228363829"
string(14) "63687228363929"
string(14) "63687228373029"
string(14) "63687228373129"
string(14) "63687228373229"
string(14) "63687228373329"
string(14) "63687228373429"
string(14) "63687228373529"
string(14) "63687228373629"
string(14) "63687228373729"
string(14) "63687228373829"
string(14) "63687228373929"
string(14) "63687228383029"
string(14) "63687228383129"
string(14) "63687228383229"
string(14) "63687228383329"
string(14) "63687228383429"
string(14) "63687228383529"
string(14) "63687228383629"
string(14) "63687228383729"
string(14) "63687228383829"
string(14) "63687228383929"
string(14) "63687228393029"
string(14) "63687228393129"
string(14) "63687228393229"
string(14) "63687228393329"
string(14) "63687228393429"
string(14) "63687228393529"
string(14) "63687228393629"
string(14) "63687228393729"
string(14) "63687228393829"
string(14) "63687228393929"
string(16) "6368722831303029"
string(16) "6368722831303129"
string(16) "6368722831303229"
string(16) "6368722831303329"
string(16) "6368722831303429"
string(16) "6368722831303529"
string(16) "6368722831303629"
string(16) "6368722831303729"
string(16) "6368722831303829"
string(16) "6368722831303929"
string(16) "6368722831313029"
string(16) "6368722831313129"
string(16) "6368722831313229"
string(16) "6368722831313329"
string(16) "6368722831313429"
string(16) "6368722831313529"
string(16) "6368722831313629"
string(16) "6368722831313729"
string(16) "6368722831313829"
string(16) "6368722831313929"
string(16) "6368722831323029"
string(16) "6368722831323129"
string(16) "6368722831323229"
string(16) "6368722831323329"
string(16) "6368722831323429"
string(16) "6368722831323529"
string(16) "6368722831323629"
string(16) "6368722831323729"
string(16) "6368722831323829"
string(16) "6368722831323929"
string(16) "6368722831333029"
string(16) "6368722831333129"
string(16) "6368722831333229"
string(16) "6368722831333329"
string(16) "6368722831333429"
string(16) "6368722831333529"
string(16) "6368722831333629"
string(16) "6368722831333729"
string(16) "6368722831333829"
string(16) "6368722831333929"
string(16) "6368722831343029"
string(16) "6368722831343129"
string(16) "6368722831343229"
string(16) "6368722831343329"
string(16) "6368722831343429"
string(16) "6368722831343529"
string(16) "6368722831343629"
string(16) "6368722831343729"
string(16) "6368722831343829"
string(16) "6368722831343929"
string(16) "6368722831353029"
string(16) "6368722831353129"
string(16) "6368722831353229"
string(16) "6368722831353329"
string(16) "6368722831353429"
string(16) "6368722831353529"
string(16) "6368722831353629"
string(16) "6368722831353729"
string(16) "6368722831353829"
string(16) "6368722831353929"
string(16) "6368722831363029"
string(16) "6368722831363129"
string(16) "6368722831363229"
string(16) "6368722831363329"
string(16) "6368722831363429"
string(16) "6368722831363529"
string(16) "6368722831363629"
string(16) "6368722831363729"
string(16) "6368722831363829"
string(16) "6368722831363929"
string(16) "6368722831373029"
string(16) "6368722831373129"
string(16) "6368722831373229"
string(16) "6368722831373329"
string(16) "6368722831373429"
string(16) "6368722831373529"
string(16) "6368722831373629"
string(16) "6368722831373729"
string(16) "6368722831373829"
string(16) "6368722831373929"
string(16) "6368722831383029"
string(16) "6368722831383129"
string(16) "6368722831383229"
string(16) "6368722831383329"
string(16) "6368722831383429"
string(16) "6368722831383529"
string(16) "6368722831383629"
string(16) "6368722831383729"
string(16) "6368722831383829"
string(16) "6368722831383929"
string(16) "6368722831393029"
string(16) "6368722831393129"
string(16) "6368722831393229"
string(16) "6368722831393329"
string(16) "6368722831393429"
string(16) "6368722831393529"
string(16) "6368722831393629"
string(16) "6368722831393729"
string(16) "6368722831393829"
string(16) "6368722831393929"
string(16) "6368722832303029"
string(16) "6368722832303129"
string(16) "6368722832303229"
string(16) "6368722832303329"
string(16) "6368722832303429"
string(16) "6368722832303529"
string(16) "6368722832303629"
string(16) "6368722832303729"
string(16) "6368722832303829"
string(16) "6368722832303929"
string(16) "6368722832313029"
string(16) "6368722832313129"
string(16) "6368722832313229"
string(16) "6368722832313329"
string(16) "6368722832313429"
string(16) "6368722832313529"
string(16) "6368722832313629"
string(16) "6368722832313729"
string(16) "6368722832313829"
string(16) "6368722832313929"
string(16) "6368722832323029"
string(16) "6368722832323129"
string(16) "6368722832323229"
string(16) "6368722832323329"
string(16) "6368722832323429"
string(16) "6368722832323529"
string(16) "6368722832323629"
string(16) "6368722832323729"
string(16) "6368722832323829"
string(16) "6368722832323929"
string(16) "6368722832333029"
string(16) "6368722832333129"
string(16) "6368722832333229"
string(16) "6368722832333329"
string(16) "6368722832333429"
string(16) "6368722832333529"
string(16) "6368722832333629"
string(16) "6368722832333729"
string(16) "6368722832333829"
string(16) "6368722832333929"
string(16) "6368722832343029"
string(16) "6368722832343129"
string(16) "6368722832343229"
string(16) "6368722832343329"
string(16) "6368722832343429"
string(16) "6368722832343529"
string(16) "6368722832343629"
string(16) "6368722832343729"
string(16) "6368722832343829"
string(16) "6368722832343929"
string(16) "6368722832353029"
string(16) "6368722832353129"
string(16) "6368722832353229"
string(16) "6368722832353329"
string(16) "6368722832353429"
string(16) "6368722832353529"

*** Testing htmlentities() with NULL as first,second and third argument ***
string(42) "&sbquo;&dagger;&trade;&Yuml;&euro;&sbquo;�"
string(0) ""
string(0) ""
string(0) ""
string(0) ""

*** Checking for proper memory allocation with long string ***
string(198) "&sbquo;&dagger;&trade;&Yuml;&euro;&sbquo;&dagger;&bdquo;&euro;&permil;&hellip;&fnof;&dagger;&bdquo;&euro;&lsquo;&fnof;&lsquo;&dagger;&Dagger;&hellip;&dagger;&circ;&sbquo;&permil;&rsquo;&lsquo;&fnof;"

*** Testing a normal string with htmlentities() ***
string(42) "&lt;html&gt; This is a test! &lt;/html&gt;"

*** Testing htmlentites() on a quote ***
string(36) "A 'quote' is &lt;b&gt;bold&lt;/b&gt;"
string(46) "A &#039;quote&#039; is &lt;b&gt;bold&lt;/b&gt;"
string(36) "A 'quote' is &lt;b&gt;bold&lt;/b&gt;"
string(36) "A 'quote' is &lt;b&gt;bold&lt;/b&gt;"

*** Testing error conditions ***

Warning: htmlentities() expects at least 1 parameter, 0 given in %s on line %d
NULL
string(14) "&bdquo;&lsquo;"
Done
