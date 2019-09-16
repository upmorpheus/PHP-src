<?php

$testsDir = __DIR__ . '/../../../Zend/tests/';
$it = new RecursiveIteratorIterator(
    new RecursiveDirectoryIterator($testsDir),
    RecursiveIteratorIterator::LEAVES_ONLY
);

$corpusDir = __DIR__ . '/parser';
@mkdir($corpusDir);

foreach ($it as $file) {
    if (!preg_match('/\.phpt$/', $file)) continue;
    $code = file_get_contents($file);
    if (!preg_match('/--FILE--(.*)--EXPECT/s', $code, $matches)) continue;
    $code = $matches[1];

    $outFile = str_replace($testsDir, '', $file);
    $outFile = str_replace('/', '_', $outFile);
    $outFile = $corpusDir . '/' . $outFile;
    file_put_contents($outFile, $code);
}
