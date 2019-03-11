--TEST--
openssl_pkey_derive() DH
--SKIPIF--
<?php if (!extension_loaded("openssl")) print "skip"; ?>
--FILE--
<?php

$priv = openssl_pkey_get_private("-----BEGIN PRIVATE KEY-----
MIICJgIBADCCARcGCSqGSIb3DQEDATCCAQgCggEBAJLxRCaZ933uW+AXmabHFDDy
upojBIRlbmQLJZfigDaSA1f9YOTsIv+WwVFTX/J1mtCyx9uBcz0Nt2kmVwxWuc2f
VtCEMPsmLsVXX7xRUFLpyX1Y1IYGBVXQOoOvLWYQjpZgnx47Pkh1Ok1+smffztfC
0DCNt4KorWrbsPcmqBejXHN79KvWFjZmXOksRiNu/Bn76RiqvofC4z8Ri3kHXQG2
197JGZzzFXHadGC3xbkg8UxsNbYhVMKbm0iANfafUH7/hoS9UjAVQYtvwe7YNiW/
HnyfVCrKwcc7sadd8Iphh+3lf5P1AhaQEAMytanrzq9RDXKBxuvpSJifRYasZYsC
AQIEggEEAoIBAGwAYC2E81Y1U2Aox0U7u1+vBcbht/OO87tutMvc4NTLf6NLPHsW
cPqBixs+3rSn4fADzAIvdLBmogjtiIZoB6qyHrllF/2xwTVGEeYaZIupQH3bMK2b
6eUvnpuu4Ytksiz6VpXBBRMrIsj3frM+zUtnq8vKUr+TbjV2qyKR8l3eNDwzqz30
dlbKh9kIhZafclHfRVfyp+fVSKPfgrRAcLUgAbsVjOjPeJ90xQ4DTMZ6vjiv6tHM
hkSjJIcGhRtSBzVF/cT38GyCeTmiIA/dRz2d70lWrqDQCdp9ArijgnpjNKAAulSY
CirnMsGZTDGmLOHg4xOZ5FEAzZI2sFNLlcw=
-----END PRIVATE KEY-----
");

$pub = openssl_pkey_get_public("-----BEGIN PUBLIC KEY-----
MIICJDCCARcGCSqGSIb3DQEDATCCAQgCggEBAJLxRCaZ933uW+AXmabHFDDyupoj
BIRlbmQLJZfigDaSA1f9YOTsIv+WwVFTX/J1mtCyx9uBcz0Nt2kmVwxWuc2fVtCE
MPsmLsVXX7xRUFLpyX1Y1IYGBVXQOoOvLWYQjpZgnx47Pkh1Ok1+smffztfC0DCN
t4KorWrbsPcmqBejXHN79KvWFjZmXOksRiNu/Bn76RiqvofC4z8Ri3kHXQG2197J
GZzzFXHadGC3xbkg8UxsNbYhVMKbm0iANfafUH7/hoS9UjAVQYtvwe7YNiW/Hnyf
VCrKwcc7sadd8Iphh+3lf5P1AhaQEAMytanrzq9RDXKBxuvpSJifRYasZYsCAQID
ggEFAAKCAQAiCSBpxvGgsTorxAWtcAlSmzAJnJxFgSPef0g7OjhESytnc8G2QYmx
ovMt5KVergcitztWh08hZQUdAYm4rI+zMlAFDdN8LWwBT/mGKSzRkWeprd8E7mvy
ucqC1YXCMqmIwPySvLQUB/Dl8kgau7BLAnIJm8VP+MVrn8g9gghD0qRCgPgtEaDV
vocfgnOU43rhKnIgO0cHOKtw2qybSFB8QuZrYugq4j8Bwkrzh6rdMMeyMl/ej5Aj
c0wamOzuBDtXt0T9+Fx3khHaowjCc7xJZRgZCxg43SbqMWJ9lUg94I7+LTX61Gyv
dtlkbGbtoDOnxeNnN93gwQZngGYZYciu
-----END PUBLIC KEY-----
");

echo bin2hex(openssl_pkey_derive($pub,$priv));
echo "\n";
?>
--EXPECT--
10aed66ad96a65f50543aa9adbc18ea169bf98521c682c49fb8b7daeb9e8fbe6b9a800199ffe1123cc36fc358829cbbc5d21bf1eb8ce3cf644538b357f478361a284c27fbe31fc94d431562786dd7314613cd70e6d76ca1ab3c1f31556ed07162f243dcc1a43ea98c454fb6e891eaec7a14158d54cd33d3fbbbc75f1ea8ff5deaab25d5deb657c7c43004252df301b195207d01614e7cb833e0e8d785ba2ecfe16ad7a9634784fdb8db8afe049476b58743575725ee99c761a59a7d7b9e709fff84c8d427e2bc07953a7c2408eb3f8f7e0ebc2f901c6889955874ae79a3de19921757d69424145a35dbe5af778b080dada55bdfce8fb0319f2de39110f58e05d
