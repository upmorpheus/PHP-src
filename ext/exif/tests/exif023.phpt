--TEST--
Check for exif_read_data, TIFF with IFD, EXIF and GPS data in Motorola byte-order.
--SKIPIF--
<?php if (!extension_loaded('exif')) print 'skip exif extension not available';?>
--INI--
output_handler=
zlib.output_compression=0
--FILE--
<?php
var_dump(exif_read_data(__DIR__.'/image023.tiff'));
?>
--EXPECTF--
array(45) {
  ["FileName"]=>
  string(13) "image023.tiff"
  ["FileDateTime"]=>
  int(%d)
  ["FileSize"]=>
  int(%d)
  ["FileType"]=>
  int(8)
  ["MimeType"]=>
  string(10) "image/tiff"
  ["SectionsFound"]=>
  string(24) "ANY_TAG, IFD0, EXIF, GPS"
  ["COMPUTED"]=>
  array(9) {
    ["html"]=>
    string(20) "width="1" height="1""
    ["Height"]=>
    int(1)
    ["Width"]=>
    int(1)
    ["IsColor"]=>
    int(1)
    ["ByteOrderMotorola"]=>
    int(1)
    ["ApertureFNumber"]=>
    string(5) "f/8.0"
    ["Copyright"]=>
    string(24) "Eric Stewart, Hex Editor"
    ["Copyright.Photographer"]=>
    string(12) "Eric Stewart"
    ["Copyright.Editor"]=>
    string(10) "Hex Editor"
  }
  ["ImageWidth"]=>
  int(1)
  ["ImageLength"]=>
  int(1)
  ["BitsPerSample"]=>
  int(8)
  ["Compression"]=>
  int(5)
  ["PhotometricInterpretation"]=>
  int(3)
  ["ImageDescription"]=>
  string(15) "My description."
  ["Make"]=>
  string(11) "OpenShutter"
  ["Model"]=>
  string(8) "OS 1.0.0"
  ["StripOffsets"]=>
  int(2278)
  ["SamplesPerPixel"]=>
  int(1)
  ["RowsPerStrip"]=>
  int(8)
  ["StripByteCounts"]=>
  int(4)
  ["XResolution"]=>
  string(17) "381681664/2097152"
  ["YResolution"]=>
  string(17) "381681664/2097152"
  ["PlanarConfiguration"]=>
  int(1)
  ["ResolutionUnit"]=>
  int(2)
  ["Artist"]=>
  string(12) "Eric Stewart"
  ["ColorMap"]=>
  array(768) {
    [0]=>
    int(0)
    [1]=>
    int(65280)
    [2]=>
    int(32512)
    [3]=>
    int(49152)
    [4]=>
    int(99)
    [5]=>
    int(115)
    [6]=>
    int(116)
    [7]=>
    int(101)
    [8]=>
    int(119)
    [9]=>
    int(97)
    [10]=>
    int(114)
    [11]=>
    int(116)
    [12]=>
    int(0)
    [13]=>
    int(0)
    [14]=>
    int(0)
    [15]=>
    int(0)
    [16]=>
    int(0)
    [17]=>
    int(0)
    [18]=>
    int(0)
    [19]=>
    int(0)
    [20]=>
    int(0)
    [21]=>
    int(0)
    [22]=>
    int(0)
    [23]=>
    int(0)
    [24]=>
    int(0)
    [25]=>
    int(0)
    [26]=>
    int(0)
    [27]=>
    int(0)
    [28]=>
    int(0)
    [29]=>
    int(0)
    [30]=>
    int(0)
    [31]=>
    int(0)
    [32]=>
    int(0)
    [33]=>
    int(0)
    [34]=>
    int(0)
    [35]=>
    int(0)
    [36]=>
    int(0)
    [37]=>
    int(0)
    [38]=>
    int(0)
    [39]=>
    int(0)
    [40]=>
    int(0)
    [41]=>
    int(0)
    [42]=>
    int(0)
    [43]=>
    int(0)
    [44]=>
    int(0)
    [45]=>
    int(0)
    [46]=>
    int(0)
    [47]=>
    int(0)
    [48]=>
    int(0)
    [49]=>
    int(0)
    [50]=>
    int(0)
    [51]=>
    int(0)
    [52]=>
    int(0)
    [53]=>
    int(0)
    [54]=>
    int(0)
    [55]=>
    int(0)
    [56]=>
    int(0)
    [57]=>
    int(0)
    [58]=>
    int(0)
    [59]=>
    int(0)
    [60]=>
    int(0)
    [61]=>
    int(0)
    [62]=>
    int(0)
    [63]=>
    int(0)
    [64]=>
    int(0)
    [65]=>
    int(0)
    [66]=>
    int(0)
    [67]=>
    int(0)
    [68]=>
    int(0)
    [69]=>
    int(0)
    [70]=>
    int(0)
    [71]=>
    int(0)
    [72]=>
    int(0)
    [73]=>
    int(0)
    [74]=>
    int(0)
    [75]=>
    int(0)
    [76]=>
    int(0)
    [77]=>
    int(0)
    [78]=>
    int(0)
    [79]=>
    int(0)
    [80]=>
    int(0)
    [81]=>
    int(0)
    [82]=>
    int(0)
    [83]=>
    int(0)
    [84]=>
    int(0)
    [85]=>
    int(0)
    [86]=>
    int(0)
    [87]=>
    int(0)
    [88]=>
    int(0)
    [89]=>
    int(0)
    [90]=>
    int(0)
    [91]=>
    int(0)
    [92]=>
    int(0)
    [93]=>
    int(0)
    [94]=>
    int(0)
    [95]=>
    int(0)
    [96]=>
    int(0)
    [97]=>
    int(0)
    [98]=>
    int(0)
    [99]=>
    int(0)
    [100]=>
    int(0)
    [101]=>
    int(0)
    [102]=>
    int(0)
    [103]=>
    int(0)
    [104]=>
    int(0)
    [105]=>
    int(0)
    [106]=>
    int(0)
    [107]=>
    int(0)
    [108]=>
    int(0)
    [109]=>
    int(0)
    [110]=>
    int(0)
    [111]=>
    int(0)
    [112]=>
    int(0)
    [113]=>
    int(0)
    [114]=>
    int(0)
    [115]=>
    int(0)
    [116]=>
    int(0)
    [117]=>
    int(0)
    [118]=>
    int(0)
    [119]=>
    int(0)
    [120]=>
    int(0)
    [121]=>
    int(0)
    [122]=>
    int(0)
    [123]=>
    int(0)
    [124]=>
    int(0)
    [125]=>
    int(0)
    [126]=>
    int(0)
    [127]=>
    int(0)
    [128]=>
    int(0)
    [129]=>
    int(0)
    [130]=>
    int(0)
    [131]=>
    int(0)
    [132]=>
    int(0)
    [133]=>
    int(0)
    [134]=>
    int(0)
    [135]=>
    int(0)
    [136]=>
    int(0)
    [137]=>
    int(0)
    [138]=>
    int(0)
    [139]=>
    int(0)
    [140]=>
    int(0)
    [141]=>
    int(0)
    [142]=>
    int(0)
    [143]=>
    int(0)
    [144]=>
    int(0)
    [145]=>
    int(0)
    [146]=>
    int(0)
    [147]=>
    int(0)
    [148]=>
    int(0)
    [149]=>
    int(0)
    [150]=>
    int(0)
    [151]=>
    int(0)
    [152]=>
    int(0)
    [153]=>
    int(0)
    [154]=>
    int(0)
    [155]=>
    int(0)
    [156]=>
    int(0)
    [157]=>
    int(0)
    [158]=>
    int(0)
    [159]=>
    int(0)
    [160]=>
    int(0)
    [161]=>
    int(0)
    [162]=>
    int(0)
    [163]=>
    int(0)
    [164]=>
    int(0)
    [165]=>
    int(0)
    [166]=>
    int(0)
    [167]=>
    int(0)
    [168]=>
    int(0)
    [169]=>
    int(0)
    [170]=>
    int(0)
    [171]=>
    int(0)
    [172]=>
    int(0)
    [173]=>
    int(0)
    [174]=>
    int(0)
    [175]=>
    int(0)
    [176]=>
    int(0)
    [177]=>
    int(0)
    [178]=>
    int(0)
    [179]=>
    int(0)
    [180]=>
    int(0)
    [181]=>
    int(0)
    [182]=>
    int(0)
    [183]=>
    int(0)
    [184]=>
    int(0)
    [185]=>
    int(0)
    [186]=>
    int(0)
    [187]=>
    int(0)
    [188]=>
    int(0)
    [189]=>
    int(0)
    [190]=>
    int(0)
    [191]=>
    int(0)
    [192]=>
    int(0)
    [193]=>
    int(0)
    [194]=>
    int(0)
    [195]=>
    int(0)
    [196]=>
    int(0)
    [197]=>
    int(0)
    [198]=>
    int(0)
    [199]=>
    int(0)
    [200]=>
    int(0)
    [201]=>
    int(0)
    [202]=>
    int(0)
    [203]=>
    int(0)
    [204]=>
    int(0)
    [205]=>
    int(0)
    [206]=>
    int(0)
    [207]=>
    int(0)
    [208]=>
    int(0)
    [209]=>
    int(0)
    [210]=>
    int(0)
    [211]=>
    int(0)
    [212]=>
    int(0)
    [213]=>
    int(0)
    [214]=>
    int(0)
    [215]=>
    int(0)
    [216]=>
    int(0)
    [217]=>
    int(0)
    [218]=>
    int(0)
    [219]=>
    int(0)
    [220]=>
    int(0)
    [221]=>
    int(0)
    [222]=>
    int(0)
    [223]=>
    int(0)
    [224]=>
    int(0)
    [225]=>
    int(0)
    [226]=>
    int(0)
    [227]=>
    int(0)
    [228]=>
    int(0)
    [229]=>
    int(0)
    [230]=>
    int(0)
    [231]=>
    int(0)
    [232]=>
    int(0)
    [233]=>
    int(0)
    [234]=>
    int(0)
    [235]=>
    int(0)
    [236]=>
    int(0)
    [237]=>
    int(0)
    [238]=>
    int(0)
    [239]=>
    int(0)
    [240]=>
    int(0)
    [241]=>
    int(0)
    [242]=>
    int(0)
    [243]=>
    int(0)
    [244]=>
    int(0)
    [245]=>
    int(0)
    [246]=>
    int(0)
    [247]=>
    int(0)
    [248]=>
    int(0)
    [249]=>
    int(0)
    [250]=>
    int(0)
    [251]=>
    int(0)
    [252]=>
    int(0)
    [253]=>
    int(0)
    [254]=>
    int(0)
    [255]=>
    int(1)
    [256]=>
    int(0)
    [257]=>
    int(65280)
    [258]=>
    int(32512)
    [259]=>
    int(49152)
    [260]=>
    int(0)
    [261]=>
    int(0)
    [262]=>
    int(0)
    [263]=>
    int(0)
    [264]=>
    int(0)
    [265]=>
    int(0)
    [266]=>
    int(0)
    [267]=>
    int(0)
    [268]=>
    int(0)
    [269]=>
    int(0)
    [270]=>
    int(0)
    [271]=>
    int(0)
    [272]=>
    int(11945)
    [273]=>
    int(1914)
    [274]=>
    int(0)
    [275]=>
    int(24609)
    [276]=>
    int(1088)
    [277]=>
    int(960)
    [278]=>
    int(0)
    [279]=>
    int(0)
    [280]=>
    int(20000)
    [281]=>
    int(8414)
    [282]=>
    int(65436)
    [283]=>
    int(0)
    [284]=>
    int(47655)
    [285]=>
    int(8)
    [286]=>
    int(37936)
    [287]=>
    int(8406)
    [288]=>
    int(0)
    [289]=>
    int(0)
    [290]=>
    int(0)
    [291]=>
    int(0)
    [292]=>
    int(0)
    [293]=>
    int(0)
    [294]=>
    int(0)
    [295]=>
    int(0)
    [296]=>
    int(0)
    [297]=>
    int(64652)
    [298]=>
    int(50264)
    [299]=>
    int(0)
    [300]=>
    int(0)
    [301]=>
    int(64887)
    [302]=>
    int(50264)
    [303]=>
    int(0)
    [304]=>
    int(25714)
    [305]=>
    int(26220)
    [306]=>
    int(17235)
    [307]=>
    int(19777)
    [308]=>
    int(65535)
    [309]=>
    int(65535)
    [310]=>
    int(65535)
    [311]=>
    int(65535)
    [312]=>
    int(65535)
    [313]=>
    int(65535)
    [314]=>
    int(65535)
    [315]=>
    int(65535)
    [316]=>
    int(501)
    [317]=>
    int(0)
    [318]=>
    int(20)
    [319]=>
    int(0)
    [320]=>
    int(0)
    [321]=>
    int(0)
    [322]=>
    int(16877)
    [323]=>
    int(0)
    [324]=>
    int(3)
    [325]=>
    int(0)
    [326]=>
    int(0)
    [327]=>
    int(0)
    [328]=>
    int(0)
    [329]=>
    int(0)
    [330]=>
    int(0)
    [331]=>
    int(0)
    [332]=>
    int(65535)
    [333]=>
    int(65535)
    [334]=>
    int(65535)
    [335]=>
    int(65535)
    [336]=>
    int(65535)
    [337]=>
    int(65535)
    [338]=>
    int(65535)
    [339]=>
    int(65535)
    [340]=>
    int(0)
    [341]=>
    int(0)
    [342]=>
    int(52840)
    [343]=>
    int(2025)
    [344]=>
    int(16)
    [345]=>
    int(57377)
    [346]=>
    int(1024)
    [347]=>
    int(960)
    [348]=>
    int(0)
    [349]=>
    int(0)
    [350]=>
    int(27136)
    [351]=>
    int(8414)
    [352]=>
    int(65436)
    [353]=>
    int(0)
    [354]=>
    int(47655)
    [355]=>
    int(8)
    [356]=>
    int(62400)
    [357]=>
    int(8407)
    [358]=>
    int(0)
    [359]=>
    int(0)
    [360]=>
    int(0)
    [361]=>
    int(0)
    [362]=>
    int(0)
    [363]=>
    int(0)
    [364]=>
    int(0)
    [365]=>
    int(0)
    [366]=>
    int(0)
    [367]=>
    int(64857)
    [368]=>
    int(50264)
    [369]=>
    int(0)
    [370]=>
    int(0)
    [371]=>
    int(64892)
    [372]=>
    int(50264)
    [373]=>
    int(0)
    [374]=>
    int(25714)
    [375]=>
    int(26220)
    [376]=>
    int(17235)
    [377]=>
    int(19777)
    [378]=>
    int(65535)
    [379]=>
    int(65535)
    [380]=>
    int(65535)
    [381]=>
    int(65535)
    [382]=>
    int(65535)
    [383]=>
    int(65535)
    [384]=>
    int(65535)
    [385]=>
    int(65535)
    [386]=>
    int(501)
    [387]=>
    int(0)
    [388]=>
    int(20)
    [389]=>
    int(0)
    [390]=>
    int(0)
    [391]=>
    int(0)
    [392]=>
    int(16877)
    [393]=>
    int(0)
    [394]=>
    int(3)
    [395]=>
    int(0)
    [396]=>
    int(0)
    [397]=>
    int(0)
    [398]=>
    int(0)
    [399]=>
    int(0)
    [400]=>
    int(0)
    [401]=>
    int(0)
    [402]=>
    int(65535)
    [403]=>
    int(65535)
    [404]=>
    int(65535)
    [405]=>
    int(65535)
    [406]=>
    int(65535)
    [407]=>
    int(65535)
    [408]=>
    int(65535)
    [409]=>
    int(65535)
    [410]=>
    int(0)
    [411]=>
    int(0)
    [412]=>
    int(53440)
    [413]=>
    int(2025)
    [414]=>
    int(16)
    [415]=>
    int(57377)
    [416]=>
    int(1024)
    [417]=>
    int(960)
    [418]=>
    int(0)
    [419]=>
    int(0)
    [420]=>
    int(41120)
    [421]=>
    int(9024)
    [422]=>
    int(65436)
    [423]=>
    int(0)
    [424]=>
    int(47655)
    [425]=>
    int(8)
    [426]=>
    int(24480)
    [427]=>
    int(8404)
    [428]=>
    int(0)
    [429]=>
    int(0)
    [430]=>
    int(0)
    [431]=>
    int(0)
    [432]=>
    int(0)
    [433]=>
    int(0)
    [434]=>
    int(0)
    [435]=>
    int(0)
    [436]=>
    int(0)
    [437]=>
    int(21315)
    [438]=>
    int(50294)
    [439]=>
    int(0)
    [440]=>
    int(0)
    [441]=>
    int(53635)
    [442]=>
    int(50294)
    [443]=>
    int(0)
    [444]=>
    int(25714)
    [445]=>
    int(26220)
    [446]=>
    int(17235)
    [447]=>
    int(19777)
    [448]=>
    int(65535)
    [449]=>
    int(65535)
    [450]=>
    int(65535)
    [451]=>
    int(65535)
    [452]=>
    int(65535)
    [453]=>
    int(65535)
    [454]=>
    int(65535)
    [455]=>
    int(65535)
    [456]=>
    int(501)
    [457]=>
    int(0)
    [458]=>
    int(20)
    [459]=>
    int(0)
    [460]=>
    int(0)
    [461]=>
    int(0)
    [462]=>
    int(16877)
    [463]=>
    int(0)
    [464]=>
    int(3)
    [465]=>
    int(0)
    [466]=>
    int(0)
    [467]=>
    int(0)
    [468]=>
    int(0)
    [469]=>
    int(0)
    [470]=>
    int(0)
    [471]=>
    int(0)
    [472]=>
    int(65535)
    [473]=>
    int(65535)
    [474]=>
    int(65535)
    [475]=>
    int(65535)
    [476]=>
    int(65535)
    [477]=>
    int(65535)
    [478]=>
    int(65535)
    [479]=>
    int(65535)
    [480]=>
    int(0)
    [481]=>
    int(0)
    [482]=>
    int(54028)
    [483]=>
    int(2772)
    [484]=>
    int(16)
    [485]=>
    int(57377)
    [486]=>
    int(1024)
    [487]=>
    int(960)
    [488]=>
    int(0)
    [489]=>
    int(0)
    [490]=>
    int(42384)
    [491]=>
    int(8408)
    [492]=>
    int(65436)
    [493]=>
    int(0)
    [494]=>
    int(47655)
    [495]=>
    int(8)
    [496]=>
    int(1136)
    [497]=>
    int(8348)
    [498]=>
    int(0)
    [499]=>
    int(0)
    [500]=>
    int(0)
    [501]=>
    int(0)
    [502]=>
    int(0)
    [503]=>
    int(0)
    [504]=>
    int(0)
    [505]=>
    int(0)
    [506]=>
    int(0)
    [507]=>
    int(12326)
    [508]=>
    int(50261)
    [509]=>
    int(0)
    [510]=>
    int(0)
    [511]=>
    int(12326)
    [512]=>
    int(0)
    [513]=>
    int(65280)
    [514]=>
    int(32512)
    [515]=>
    int(49152)
    [516]=>
    int(0)
    [517]=>
    int(0)
    [518]=>
    int(22663)
    [519]=>
    int(2)
    [520]=>
    int(0)
    [521]=>
    int(0)
    [522]=>
    int(24576)
    [523]=>
    int(2)
    [524]=>
    int(0)
    [525]=>
    int(0)
    [526]=>
    int(501)
    [527]=>
    int(0)
    [528]=>
    int(20)
    [529]=>
    int(0)
    [530]=>
    int(0)
    [531]=>
    int(0)
    [532]=>
    int(33188)
    [533]=>
    int(0)
    [534]=>
    int(0)
    [535]=>
    int(0)
    [536]=>
    int(0)
    [537]=>
    int(0)
    [538]=>
    int(0)
    [539]=>
    int(0)
    [540]=>
    int(0)
    [541]=>
    int(0)
    [542]=>
    int(0)
    [543]=>
    int(0)
    [544]=>
    int(0)
    [545]=>
    int(0)
    [546]=>
    int(0)
    [547]=>
    int(0)
    [548]=>
    int(0)
    [549]=>
    int(0)
    [550]=>
    int(0)
    [551]=>
    int(0)
    [552]=>
    int(51766)
    [553]=>
    int(1946)
    [554]=>
    int(0)
    [555]=>
    int(24609)
    [556]=>
    int(1088)
    [557]=>
    int(960)
    [558]=>
    int(0)
    [559]=>
    int(0)
    [560]=>
    int(0)
    [561]=>
    int(0)
    [562]=>
    int(25116)
    [563]=>
    int(2012)
    [564]=>
    int(0)
    [565]=>
    int(0)
    [566]=>
    int(0)
    [567]=>
    int(0)
    [568]=>
    int(0)
    [569]=>
    int(0)
    [570]=>
    int(0)
    [571]=>
    int(0)
    [572]=>
    int(0)
    [573]=>
    int(0)
    [574]=>
    int(0)
    [575]=>
    int(0)
    [576]=>
    int(0)
    [577]=>
    int(0)
    [578]=>
    int(0)
    [579]=>
    int(0)
    [580]=>
    int(0)
    [581]=>
    int(0)
    [582]=>
    int(0)
    [583]=>
    int(0)
    [584]=>
    int(0)
    [585]=>
    int(0)
    [586]=>
    int(0)
    [587]=>
    int(0)
    [588]=>
    int(0)
    [589]=>
    int(0)
    [590]=>
    int(0)
    [591]=>
    int(0)
    [592]=>
    int(0)
    [593]=>
    int(0)
    [594]=>
    int(0)
    [595]=>
    int(0)
    [596]=>
    int(0)
    [597]=>
    int(0)
    [598]=>
    int(0)
    [599]=>
    int(0)
    [600]=>
    int(0)
    [601]=>
    int(0)
    [602]=>
    int(0)
    [603]=>
    int(0)
    [604]=>
    int(0)
    [605]=>
    int(0)
    [606]=>
    int(0)
    [607]=>
    int(0)
    [608]=>
    int(0)
    [609]=>
    int(0)
    [610]=>
    int(0)
    [611]=>
    int(0)
    [612]=>
    int(0)
    [613]=>
    int(0)
    [614]=>
    int(0)
    [615]=>
    int(0)
    [616]=>
    int(0)
    [617]=>
    int(0)
    [618]=>
    int(0)
    [619]=>
    int(0)
    [620]=>
    int(0)
    [621]=>
    int(0)
    [622]=>
    int(0)
    [623]=>
    int(0)
    [624]=>
    int(0)
    [625]=>
    int(0)
    [626]=>
    int(0)
    [627]=>
    int(0)
    [628]=>
    int(0)
    [629]=>
    int(0)
    [630]=>
    int(0)
    [631]=>
    int(0)
    [632]=>
    int(0)
    [633]=>
    int(0)
    [634]=>
    int(0)
    [635]=>
    int(0)
    [636]=>
    int(0)
    [637]=>
    int(0)
    [638]=>
    int(0)
    [639]=>
    int(0)
    [640]=>
    int(0)
    [641]=>
    int(0)
    [642]=>
    int(0)
    [643]=>
    int(0)
    [644]=>
    int(0)
    [645]=>
    int(0)
    [646]=>
    int(0)
    [647]=>
    int(0)
    [648]=>
    int(0)
    [649]=>
    int(0)
    [650]=>
    int(0)
    [651]=>
    int(0)
    [652]=>
    int(0)
    [653]=>
    int(0)
    [654]=>
    int(0)
    [655]=>
    int(0)
    [656]=>
    int(0)
    [657]=>
    int(0)
    [658]=>
    int(0)
    [659]=>
    int(0)
    [660]=>
    int(0)
    [661]=>
    int(0)
    [662]=>
    int(0)
    [663]=>
    int(0)
    [664]=>
    int(0)
    [665]=>
    int(0)
    [666]=>
    int(0)
    [667]=>
    int(0)
    [668]=>
    int(0)
    [669]=>
    int(0)
    [670]=>
    int(0)
    [671]=>
    int(0)
    [672]=>
    int(0)
    [673]=>
    int(0)
    [674]=>
    int(0)
    [675]=>
    int(0)
    [676]=>
    int(0)
    [677]=>
    int(0)
    [678]=>
    int(0)
    [679]=>
    int(0)
    [680]=>
    int(0)
    [681]=>
    int(0)
    [682]=>
    int(0)
    [683]=>
    int(0)
    [684]=>
    int(0)
    [685]=>
    int(0)
    [686]=>
    int(0)
    [687]=>
    int(0)
    [688]=>
    int(0)
    [689]=>
    int(0)
    [690]=>
    int(0)
    [691]=>
    int(0)
    [692]=>
    int(0)
    [693]=>
    int(0)
    [694]=>
    int(0)
    [695]=>
    int(0)
    [696]=>
    int(0)
    [697]=>
    int(0)
    [698]=>
    int(0)
    [699]=>
    int(0)
    [700]=>
    int(0)
    [701]=>
    int(0)
    [702]=>
    int(0)
    [703]=>
    int(0)
    [704]=>
    int(0)
    [705]=>
    int(0)
    [706]=>
    int(0)
    [707]=>
    int(0)
    [708]=>
    int(0)
    [709]=>
    int(0)
    [710]=>
    int(0)
    [711]=>
    int(0)
    [712]=>
    int(0)
    [713]=>
    int(0)
    [714]=>
    int(0)
    [715]=>
    int(0)
    [716]=>
    int(0)
    [717]=>
    int(0)
    [718]=>
    int(0)
    [719]=>
    int(0)
    [720]=>
    int(0)
    [721]=>
    int(0)
    [722]=>
    int(0)
    [723]=>
    int(0)
    [724]=>
    int(0)
    [725]=>
    int(0)
    [726]=>
    int(0)
    [727]=>
    int(0)
    [728]=>
    int(0)
    [729]=>
    int(0)
    [730]=>
    int(0)
    [731]=>
    int(0)
    [732]=>
    int(0)
    [733]=>
    int(0)
    [734]=>
    int(0)
    [735]=>
    int(0)
    [736]=>
    int(0)
    [737]=>
    int(0)
    [738]=>
    int(0)
    [739]=>
    int(0)
    [740]=>
    int(0)
    [741]=>
    int(0)
    [742]=>
    int(0)
    [743]=>
    int(0)
    [744]=>
    int(0)
    [745]=>
    int(0)
    [746]=>
    int(0)
    [747]=>
    int(0)
    [748]=>
    int(0)
    [749]=>
    int(0)
    [750]=>
    int(0)
    [751]=>
    int(0)
    [752]=>
    int(0)
    [753]=>
    int(0)
    [754]=>
    int(0)
    [755]=>
    int(0)
    [756]=>
    int(0)
    [757]=>
    int(0)
    [758]=>
    int(0)
    [759]=>
    int(0)
    [760]=>
    int(0)
    [761]=>
    int(0)
    [762]=>
    int(0)
    [763]=>
    int(0)
    [764]=>
    int(0)
    [765]=>
    int(0)
    [766]=>
    int(0)
    [767]=>
    int(0)
  }
  ["Copyright"]=>
  string(12) "Eric Stewart"
  ["ExposureTime"]=>
  string(5) "1/125"
  ["FNumber"]=>
  string(3) "8/1"
  ["ISOSpeedRatings"]=>
  int(80)
  ["DateTimeOriginal"]=>
  string(19) "2008:06:19 01:47:53"
  ["DateTimeDigitized"]=>
  string(19) "2008:06:19 01:47:53"
  ["MeteringMode"]=>
  int(5)
  ["LightSource"]=>
  int(4)
  ["Flash"]=>
  int(7)
  ["FocalLength"]=>
  string(4) "29/5"
  ["ExifImageWidth"]=>
  int(1)
  ["ExifImageLength"]=>
  int(1)
  ["GPSVersion"]=>
  string(4) "  "
  ["GPSLatitudeRef"]=>
  string(1) "N"
  ["GPSLatitude"]=>
  array(3) {
    [0]=>
    string(4) "33/1"
    [1]=>
    string(4) "37/1"
    [2]=>
    string(3) "0/1"
  }
  ["GPSLongitudeRef"]=>
  string(1) "W"
  ["GPSLongitude"]=>
  array(3) {
    [0]=>
    string(4) "84/1"
    [1]=>
    string(3) "7/1"
    [2]=>
    string(3) "0/1"
  }
  ["GPSAltitudeRef"]=>
  string(1) " "
  ["GPSAltitude"]=>
  string(5) "295/1"
  ["GPSTimeStamp"]=>
  array(3) {
    [0]=>
    string(3) "1/1"
    [1]=>
    string(4) "47/1"
    [2]=>
    string(4) "53/1"
  }
}
--CREDITS--
Eric Stewart <ericleestewart@gmail.com>
