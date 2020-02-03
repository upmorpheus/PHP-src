--TEST--
Hash: FNV164 algorithm
--FILE--
<?php

function R10($t) {
    return str_repeat($t, 10);
}

function R500($t) {
    return str_repeat($t, 500);
}

$tests = array(
    array( "", "cbf29ce484222325" ),
    array( "a", "af63bd4c8601b7be" ),
    array( "b", "af63bd4c8601b7bd" ),
    array( "c", "af63bd4c8601b7bc" ),
    array( "d", "af63bd4c8601b7bb" ),
    array( "e", "af63bd4c8601b7ba" ),
    array( "f", "af63bd4c8601b7b9" ),
    array( "fo", "08326207b4eb2f34" ),
    array( "foo", "d8cbc7186ba13533" ),
    array( "foob", "0378817ee2ed65cb" ),
    array( "fooba", "d329d59b9963f790" ),
    array( "foobar", "340d8765a4dda9c2" ),
    array( "\0", "af63bd4c8601b7df" ),
    array( "a\0", "08326707b4eb37da" ),
    array( "b\0", "08326607b4eb3627" ),
    array( "c\0", "08326507b4eb3474" ),
    array( "d\0", "08326407b4eb32c1" ),
    array( "e\0", "08326307b4eb310e" ),
    array( "f\0", "08326207b4eb2f5b" ),
    array( "fo\0", "d8cbc7186ba1355c" ),
    array( "foo\0", "0378817ee2ed65a9" ),
    array( "foob\0", "d329d59b9963f7f1" ),
    array( "fooba\0", "340d8765a4dda9b0" ),
    array( "foobar\0", "50a6d3b724a774a6" ),
    array( "ch", "08326507b4eb341c" ),
    array( "cho", "d8d5c8186ba98bfb" ),
    array( "chon", "1ccefc7ef118dbef" ),
    array( "chong", "0c92fab3ad3db77a" ),
    array( "chongo", "9b77794f5fdec421" ),
    array( "chongo ", "0ac742dfe7874433" ),
    array( "chongo w", "d7dad5766ad8e2de" ),
    array( "chongo wa", "a1bb96378e897f5b" ),
    array( "chongo was", "5b3f9b6733a367d2" ),
    array( "chongo was ", "b07ce25cbea969f6" ),
    array( "chongo was h", "8d9e9997f9df0d6a" ),
    array( "chongo was he", "838c673d9603cb7b" ),
    array( "chongo was her", "8b5ee8a5e872c273" ),
    array( "chongo was here", "4507c4e9fb00690c" ),
    array( "chongo was here!", "4c9ca59581b27f45" ),
    array( "chongo was here!\n", "e0aca20b624e4235" ),
    array( "ch\0", "d8d5c8186ba98b94" ),
    array( "cho\0", "1ccefc7ef118db81" ),
    array( "chon\0", "0c92fab3ad3db71d" ),
    array( "chong\0", "9b77794f5fdec44e" ),
    array( "chongo\0", "0ac742dfe7874413" ),
    array( "chongo \0", "d7dad5766ad8e2a9" ),
    array( "chongo w\0", "a1bb96378e897f3a" ),
    array( "chongo wa\0", "5b3f9b6733a367a1" ),
    array( "chongo was\0", "b07ce25cbea969d6" ),
    array( "chongo was \0", "8d9e9997f9df0d02" ),
    array( "chongo was h\0", "838c673d9603cb1e" ),
    array( "chongo was he\0", "8b5ee8a5e872c201" ),
    array( "chongo was her\0", "4507c4e9fb006969" ),
    array( "chongo was here\0", "4c9ca59581b27f64" ),
    array( "chongo was here!\0", "e0aca20b624e423f" ),
    array( "chongo was here!\n\0", "13998e580afa800f" ),
    array( "cu", "08326507b4eb3401" ),
    array( "cur", "d8d5ad186ba95dc1" ),
    array( "curd", "1c72e17ef0ca4e97" ),
    array( "curds", "2183c1b327c38ae6" ),
    array( "curds ", "b66d096c914504f2" ),
    array( "curds a", "404bf57ad8476757" ),
    array( "curds an", "887976bd815498bb" ),
    array( "curds and", "3afd7f02c2bf85a5" ),
    array( "curds and ", "fc4476b0eb70177f" ),
    array( "curds and w", "186d2da00f77ecba" ),
    array( "curds and wh", "f97140fa48c74066" ),
    array( "curds and whe", "a2b1cf49aa926d37" ),
    array( "curds and whey", "0690712cd6cf940c" ),
    array( "curds and whey\n", "f7045b3102b8906e" ),
    array( "cu\0", "d8d5ad186ba95db3" ),
    array( "cur\0", "1c72e17ef0ca4ef3" ),
    array( "curd\0", "2183c1b327c38a95" ),
    array( "curds\0", "b66d096c914504d2" ),
    array( "curds \0", "404bf57ad8476736" ),
    array( "curds a\0", "887976bd815498d5" ),
    array( "curds an\0", "3afd7f02c2bf85c1" ),
    array( "curds and\0", "fc4476b0eb70175f" ),
    array( "curds and \0", "186d2da00f77eccd" ),
    array( "curds and w\0", "f97140fa48c7400e" ),
    array( "curds and wh\0", "a2b1cf49aa926d52" ),
    array( "curds and whe\0", "0690712cd6cf9475" ),
    array( "curds and whey\0", "f7045b3102b89064" ),
    array( "curds and whey\n\0", "74f762479f9d6aea" ),
    array( "line 1\nline 2\nline 3", "a64e5f36c9e2b0e3" ),
    array( "chongo <Landon Curt Noll> /\\../\\", "8fd0680da3088a04" ),
    array( "chongo <Landon Curt Noll> /\\../\\\0", "67aad32c078284cc" ),
    array( "chongo (Landon Curt Noll) /\\../\\", "b37d55d81c57b331" ),
    array( "chongo (Landon Curt Noll) /\\../\\\0", "55ac0f3829057c43" ),
    array( "http://antwrp.gsfc.nasa.gov/apod/astropix.html", "cb27f4b8e1b6cc20" ),
    array( "http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash", "26caf88bcbef2d19" ),
    array( "http://epod.usra.edu/", "8e6e063b97e61b8f" ),
    array( "http://exoplanet.eu/", "b42750f7f3b7c37e" ),
    array( "http://hvo.wr.usgs.gov/cam3/", "f3c6ba64cf7ca99b" ),
    array( "http://hvo.wr.usgs.gov/cams/HMcam/", "ebfb69b427ea80fe" ),
    array( "http://hvo.wr.usgs.gov/kilauea/update/deformation.html", "39b50c3ed970f46c" ),
    array( "http://hvo.wr.usgs.gov/kilauea/update/images.html", "5b9b177aa3eb3e8a" ),
    array( "http://hvo.wr.usgs.gov/kilauea/update/maps.html", "6510063ecf4ec903" ),
    array( "http://hvo.wr.usgs.gov/volcanowatch/current_issue.html", "2b3bbd2c00797c7a" ),
    array( "http://neo.jpl.nasa.gov/risk/", "f1d6204ff5cb4aa7" ),
    array( "http://norvig.com/21-days.html", "4836e27ccf099f38" ),
    array( "http://primes.utm.edu/curios/home.php", "82efbb0dd073b44d" ),
    array( "http://slashdot.org/", "4a80c282ffd7d4c6" ),
    array( "http://tux.wr.usgs.gov/Maps/155.25-19.5.html", "305d1a9c9ee43bdf" ),
    array( "http://volcano.wr.usgs.gov/kilaueastatus.php", "15c366948ffc6997" ),
    array( "http://www.avo.alaska.edu/activity/Redoubt.php", "80153ae218916e7b" ),
    array( "http://www.dilbert.com/fast/", "fa23e2bdf9e2a9e1" ),
    array( "http://www.fourmilab.ch/gravitation/orbits/", "d47e8d8a2333c6de" ),
    array( "http://www.fpoa.net/", "7e128095f688b056" ),
    array( "http://www.ioccc.org/index.html", "2f5356890efcedab" ),
    array( "http://www.isthe.com/cgi-bin/number.cgi", "95c2b383014f55c5" ),
    array( "http://www.isthe.com/chongo/bio.html", "4727a5339ce6070f" ),
    array( "http://www.isthe.com/chongo/index.html", "b0555ecd575108e9" ),
    array( "http://www.isthe.com/chongo/src/calc/lucas-calc", "48d785770bb4af37" ),
    array( "http://www.isthe.com/chongo/tech/astro/venus2004.html", "09d4701c12af02b1" ),
    array( "http://www.isthe.com/chongo/tech/astro/vita.html", "79f031e78f3cf62e" ),
    array( "http://www.isthe.com/chongo/tech/comp/c/expert.html", "52a1ee85db1b5a94" ),
    array( "http://www.isthe.com/chongo/tech/comp/calc/index.html", "6bd95b2eb37fa6b8" ),
    array( "http://www.isthe.com/chongo/tech/comp/fnv/index.html", "74971b7077aef85d" ),
    array( "http://www.isthe.com/chongo/tech/math/number/howhigh.html", "b4e4fae2ffcc1aad" ),
    array( "http://www.isthe.com/chongo/tech/math/number/number.html", "2bd48bd898b8f63a" ),
    array( "http://www.isthe.com/chongo/tech/math/prime/mersenne.html", "e9966ac1556257f6" ),
    array( "http://www.isthe.com/chongo/tech/math/prime/mersenne.html#largest", "92a3d1cd078ba293" ),
    array( "http://www.lavarnd.org/cgi-bin/corpspeak.cgi", "f81175a482e20ab8" ),
    array( "http://www.lavarnd.org/cgi-bin/haiku.cgi", "5bbb3de722e73048" ),
    array( "http://www.lavarnd.org/cgi-bin/rand-none.cgi", "6b4f363492b9f2be" ),
    array( "http://www.lavarnd.org/cgi-bin/randdist.cgi", "c2d559df73d59875" ),
    array( "http://www.lavarnd.org/index.html", "f75f62284bc7a8c2" ),
    array( "http://www.lavarnd.org/what/nist-test.html", "da8dd8e116a9f1cc" ),
    array( "http://www.macosxhints.com/", "bdc1e6ab76057885" ),
    array( "http://www.mellis.com/", "fec6a4238a1224a0" ),
    array( "http://www.nature.nps.gov/air/webcams/parks/havoso2alert/havoalert.cfm", "c03f40f3223e290e" ),
    array( "http://www.nature.nps.gov/air/webcams/parks/havoso2alert/timelines_24.cfm", "1ed21673466ffda9" ),
    array( "http://www.paulnoll.com/", "df70f906bb0dd2af" ),
    array( "http://www.pepysdiary.com/", "f3dcda369f2af666" ),
    array( "http://www.sciencenews.org/index/home/activity/view", "9ebb11573cdcebde" ),
    array( "http://www.skyandtelescope.com/", "81c72d9077fedca0" ),
    array( "http://www.sput.nl/~rob/sirius.html", "0ec074a31be5fb15" ),
    array( "http://www.systemexperts.com/", "2a8b3280b6c48f20" ),
    array( "http://www.tq-international.com/phpBB3/index.php", "fd31777513309344" ),
    array( "http://www.travelquesttours.com/index.htm", "194534a86ad006b6" ),
    array( "http://www.wunderground.com/global/stations/89606.html", "3be6fdf46e0cfe12" ),
    array( R10("21701"), "017cc137a07eb057" ),
    array( R10("M21701"), "9428fc6e7d26b54d" ),
    array( R10("2^21701-1"), "9aaa2e3603ef8ad7" ),
    array( R10("\x54\xc5"), "82c6d3f3a0ccdf7d" ),
    array( R10("\xc5\x54"), "c86eeea00cf09b65" ),
    array( R10("23209"), "705f8189dbb58299" ),
    array( R10("M23209"), "415a7f554391ca69" ),
    array( R10("2^23209-1"), "cfe3d49fa2bdc555" ),
    array( R10("\x5a\xa9"), "f0f9c56039b25191" ),
    array( R10("\xa9\x5a"), "7075cb6abd1d32d9" ),
    array( R10("391581216093"), "43c94e2c8b277509" ),
    array( R10("391581*2^216093-1"), "3cbfd4e4ea670359" ),
    array( R10("\x05\xf9\x9d\x03\x4c\x81"), "c05887810f4d019d" ),
    array( R10("FEDCBA9876543210"), "14468ff93ac22dc5" ),
    array( R10("\xfe\xdc\xba\x98\x76\x54\x32\x10"), "ebed699589d99c05" ),
    array( R10("EFCDAB8967452301"), "6d99f6df321ca5d5" ),
    array( R10("\xef\xcd\xab\x89\x67\x45\x23\x01"), "0cd410d08c36d625" ),
    array( R10("0123456789ABCDEF"), "ef1b2a2c86831d35" ),
    array( R10("\x01\x23\x45\x67\x89\xab\xcd\xef"), "3b349c4d69ee5f05" ),
    array( R10("1032547698BADCFE"), "55248ce88f45f035" ),
    array( R10("\x10\x32\x54\x76\x98\xba\xdc\xfe"), "aa69ca6a18a4c885" ),
    array( R500("\x00"), "1fe3fce62bd816b5" ),
    array( R500("\x07"), "0289a488a8df69d9" ),
    array( R500("~"), "15e96e1613df98b5" ),
    array( R500("\x7f"), "e6be57375ad89b99" ),
);

$i = 0;
$pass = true;
foreach($tests as $test) {
    $result = hash('fnv164', $test[0]);
    if ($result != $test[1]) {
        echo "Iteration " . $i . " failed - expected '" . $test[1] . "', got '" . $result . "' for '" . $test[1] . "'\n";
        $pass = false;
    }
    $i++;
}

if($pass) {
    echo "PASS";
}
?>
--EXPECT--
PASS
