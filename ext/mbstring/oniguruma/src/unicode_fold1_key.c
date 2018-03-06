/* This file was converted by gperf_fold_key_conv.py
      from gperf output file. */
/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf -n -C -T -c -t -j1 -L ANSI-C -F,-1 -N unicode_fold1_key unicode_fold1_key.gperf  */
/* Computed positions: -k'1-3' */



/* This gperf source file was generated by make_unicode_fold_data.py */
#include <string.h>
#include "regenc.h"

#define TOTAL_KEYWORDS 1196
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 3
#define MIN_HASH_VALUE 6
#define MAX_HASH_VALUE 1304
/* maximum key range = 1299, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
hash(OnigCodePoint codes[])
{
  static const unsigned short asso_values[] =
    {
         7,    5,   93,    4,    1,   16,  893, 1162,  891, 1085,
       889,  380,    8,  376, 1101,  332, 1100,  329, 1099,    3,
       874,  943,  870,  914,   87,  322, 1095,  317, 1093,  579,
         2,   11, 1054,   13,  956,  902,   31,  775,  632,  343,
       619,  330,  611,  323,    0,   27,  850,  311,  599,  309,
       849, 1240,  862, 1226,  445, 1220,  591,  888,  583, 1253,
      1091, 1217, 1087, 1079, 1074,  419,  858,  845,  441,  304,
       400,  842,  394, 1067,  389, 1053,  383, 1211,  218, 1206,
       202,  436,  194, 1202,  181, 1183,  153, 1177,  376, 1166,
       166,   56,  323,    0,  571,  112,  834,  877, 1035,  609,
       123,  901,  145,  684,  115,  933,  364, 1163,  345,  603,
       311,  144,  573,  162,  563, 1188,  260,  964,  240, 1174,
       476, 1034,  453,  814,  669,  454,  815,  799,  656, 1242,
       963,  113,  806,  270,  561,  304,  795,  268, 1031,  581,
      1157,  182,   44,  467, 1154, 1305, 1042, 1305, 1028,  387,
       869, 1305,  362, 1305, 1142, 1305,  639,  234, 1060, 1305,
      1131,  381,  190,  194,  780,  185,  140,   34,  209,  173,
      1021,  369, 1017,  145, 1150,  834,  788,  820, 1011,  116,
       773,  658, 1004, 1241,  767,   64,  762,  648,  756,  510,
       999,  487,  750,  254,  740,  625,  733,  662,  728,  992,
       720,  713,  533,  986,  515,  169,  427,  702,  266,  693,
       687,  644,  337,  963,  556,  954,  524,  947,  329,  942,
       288,  931,  229,  925,  104, 1143,   80,  908,   48, 1138,
        71, 1133,   59, 1131,  295,  605,  276,  525,  547, 1107,
       540,  406,  504,  156,  920,   93,  493,   84,  676,   34,
       247,    4, 1128,   14,   18,   32,  220,    2,  131
    };
  return asso_values[(unsigned char)onig_codes_byte_at(codes, 2)+3] + asso_values[(unsigned char)onig_codes_byte_at(codes, 1)] + asso_values[(unsigned char)onig_codes_byte_at(codes, 0)];
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
int
unicode_fold1_key(OnigCodePoint codes[])
{
  static const short int wordlist[] =
    {
      -1, -1, -1, -1, -1, -1,

      3240,

      2547,

      1016,

      1772,

      1712,

      1751,

      231,

      171,

      210,

      884,

      165,

      1724,

      147,

      2085,

      183,

      2106,

      2082,

      1322,

      1262,

      1301,

      1253,

      2073,

      153,

      887,

      493,

      1274,

      156,

      3243,

      2088,

      2748,

      2826,

      2793,

      2745,

      3234,

      1760,

      2985,

      2772,

      219,

      1460,

      2829,

      159,

      2835,

      144,

      2127,

      2751,

      2601,

      1085,

      1920,

      1310,

      2730,

      1214,

      2034,

      1643,

      854,

      459,

      3456,

      80,

      2541,

      1010,

      2844,

      2283,

      1220,

      2040,

      1655,

      860,

      465,

      3468,

      92,

      1514,

      756,

      405,

      2286,

      2733,

      1217,

      2037,

      1649,

      857,

      462,

      3462,

      86,

      2394,

      2727,

      1211,

      2031,

      1637,

      851,

      456,

      3450,

      1700,

      3228,

      1742,

      2406,

      138,

      201,

      559,

      502,

      541,

      1694,

      878,

      2121,

      3507,

      132,

      514,

      2400,

      1292,

      2724,

      1208,

      2028,

      1631,

      848,

      453,

      3444,

      2388,

      2553,

      1022,

      2808,

      2565,

      1040,

      1863,

      354,

      1496,

      315,

      399,

      12,

      2562,

      1034,

      1856,

      2217,

      2268,

      309,

      1427,

      0,

      1346,

      1259,

      2079,

      2205,

      2382,

      499,

      694,

      168,

      1334,

      2637,

      1121,

      1941,

      1457,

      2577,

      387,

      1037,

      1860,

      1478,

      3072,

      312,

      31,

      6,

      2526,

      995,

      1835,

      2211,

      3066,

      288,

      1688,

      1367,

      1340,

      3501,

      126,

      2193,

      3603,

      2538,

      1007,

      1841,

      37,

      1178,

      294,

      1574,

      824,

      3138,

      3387,

      1466,

      2199,

      1373,

      3069,

      532,

      2520,

      989,

      1832,

      2949,

      3213,

      285,

      3045,

      1454,

      3597,

      2631,

      1115,

      2190,

      3354,

      2514,

      983,

      1829,

      1448,

      3225,

      282,

      3051,

      3348,

      2508,

      977,

      1826,

      631,

      676,

      279,

      643,

      2640,

      1124,

      1944,

      1463,

      3207,

      390,

      3042,

      637,

      3342,

      2502,

      971,

      1823,

      1256,

      2076,

      276,

      3132,

      496,

      3201,

      162,

      3039,

      2721,

      1205,

      2025,

      1625,

      845,

      3195,

      3438,

      3036,

      658,

      640,

      378,

      2580,

      1061,

      1884,

      3141,

      712,

      336,

      613,

      52,

      1247,

      2067,

      3189,

      2235,

      3033,

      2301,

      3540,

      2919,

      1388,

      1538,

      786,

      622,

      2376,

      1058,

      1881,

      2913,

      709,

      333,

      2700,

      46,

      2004,

      1583,

      833,

      2229,

      3396,

      2295,

      607,

      691,

      1382,

      1226,

      2046,

      1667,

      866,

      471,

      3480,

      105,

      1445,

      2916,

      1433,

      601,

      2718,

      1202,

      2022,

      1619,

      842,

      2892,

      3432,

      598,

      1223,

      2043,

      1661,

      863,

      468,

      3474,

      99,

      3315,

      2475,

      944,

      2898,

      2418,

      3255,

      264,

      3249,

      595,

      1049,

      1872,

      3591,

      2178,

      324,

      2340,

      27,

      2370,

      1439,

      2889,

      3237,

      2544,

      1013,

      1844,

      2412,

      1364,

      297,

      2715,

      1199,

      2019,

      1613,

      839,

      2886,

      3426,

      3162,

      2706,

      1190,

      2010,

      1595,

      2142,

      2883,

      3408,

      2817,

      3081,

      1046,

      1869,

      2277,

      2811,

      321,

      3549,

      3606,

      664,

      2136,

      3231,

      2787,

      3054,

      2880,

      2781,

      2364,

      1358,

      2616,

      1100,

      1935,

      1043,

      1866,

      372,

      2346,

      318,

      2853,

      18,

      1472,

      3585,

      393,

      2223,

      2532,

      1001,

      1838,

      3078,

      1352,

      291,

      3336,

      2496,

      965,

      1820,

      381,

      2196,

      3330,

      2490,

      959,

      1817,

      369,

      3324,

      2484,

      953,

      1814,

      586,

      3075,

      3318,

      2478,

      947,

      1811,

      2775,

      655,

      2187,

      2739,

      2769,

      3219,

      1682,

      3048,

      2181,

      3495,

      120,

      3579,

      3183,

      625,

      3030,

      3303,

      2463,

      932,

      3177,

      3561,

      3027,

      258,
      -1,

      3171,

      2697,

      3024,

      2001,

      1577,

      827,

      3165,

      3390,

      3021,

      3351,

      2511,

      980,

      652,
      -1,

      3312,

      2472,

      941,

      1808,

      3270,

      2430,

      899,

      1787,
      -1,

      2289,

      243,

      2928,

      2175,
      -1,

      1067,

      1890,

      2157,

      715,

      342,

      345,

      65,

      74,
      -1,

      2901,

      2247,

      2256,

      2313,

      2322,

      3198,

      1400,

      1409,
      -1,

      366,

      3159,

      706,

      3018,

      592,

      1064,

      1887,

      2997,

      700,

      339,

      589,

      59,
      -1,

      2925,
      -1,

      2241,

      3090,

      2307,

      1532,

      779,

      1394,

      2742,

      1241,

      2061,

      1697,

      881,

      483,
      -1,

      135,

      3111,

      2964,
      -1,

      2922,

      1235,

      2055,

      1685,

      875,

      480,

      3498,

      123,

      3543,

      1526,

      772,

      2694,

      2895,

      1998,

      1571,

      820,

      421,

      3384,

      3156,

      2877,

      2712,

      1196,

      2016,

      1607,

      1670,

      2874,

      3420,

      3483,

      108,

      2691,

      2871,

      1995,

      1565,

      814,

      574,

      3378,

      2868,

      1232,

      2052,

      1679,

      872,

      477,

      3492,

      117,

      1229,

      2049,

      1673,

      869,

      474,

      3486,

      111,

      2358,

      2709,

      1193,

      2013,

      1601,

      697,

      2589,

      3414,

      1908,

      1055,

      1878,

      357,
      -1,

      330,

      670,

      40,

      2550,

      1019,

      1847,

      1052,

      1875,

      300,

      1436,

      327,

      1376,

      34,

      2865,

      3282,

      2442,

      911,

      1793,

      363,

      2352,

      249,

      1370,

      3276,

      2436,

      905,

      1790,

      2169,

      3102,

      246,

      3087,

      3252,
      -1,

      3537,

      1778,

      2163,

      2574,

      237,

      3057,

      2823,

      3084,

      1664,

      3573,

      24,

      3477,

      102,

      1769,
      -1,

      1328,

      228,

      3003,

      3531,

      1361,

      2202,

      1766,

      2145,
      -1,

      225,

      3000,

      2862,

      1319,

      1544,

      792,

      2139,

      3357,
      -1,

      2991,

      1763,

      1316,

      2415,

      222,

      2859,

      2622,

      1106,

      3567,
      -1,

      2133,

      375,

      2982,

      2856,

      1592,

      1313,

      432,

      3405,

      1520,

      762,

      2979,

      679,

      2274,

      661,

      1076,

      1899,

      2850,

      724,

      1502,

      742,

      1166,

      628,

      1550,

      2976,

      2265,

      3363,

      2331,

      1070,

      1893,

      1418,

      3123,

      685,

      2343,

      71,

      1244,

      2064,

      1703,

      2253,

      487,

      2319,

      141,
      -1,

      1406,
      -1,

      2703,

      1187,

      2007,

      1589,

      9,

      565,

      3402,

      1184,

      2214,

      1586,

      836,

      429,

      3399,

      1343,

      2937,

      634,

      1181,

      556,

      1580,

      830,

      425,

      3393,
      -1,

      3510,

      2904,

      553,

      2931,

      1172,
      -1,

      1562,

      810,

      417,

      3375,

      2688,
      -1,

      1992,

      1559,

      807,

      550,

      3372,
      -1,

      2685,

      3558,

      1989,

      1553,

      800,

      2682,

      3366,

      1986,

      1547,

      796,
      -1,

      3360,

      2679,

      1163,

      1983,

      1541,

      789,

      414,
      -1,

      3516,
      -1,

      673,

      2676,

      1160,

      1980,

      1535,

      783,

      411,

      2670,

      1154,

      1974,

      1523,

      767,

      408,

      2667,

      1151,

      1971,

      1517,

      759,

      2664,

      1148,

      1968,

      1511,

      752,

      3555,

      2658,

      1142,

      1962,

      1499,

      739,

      3552,

      2970,

      2634,

      1118,

      1938,

      1451,

      2271,

      384,

      2130,

      3546,

      2652,

      1136,

      1956,

      1487,

      730,

      396,
      -1,

      2592,

      3153,

      1911,

      3528,
      -1,

      360,

      3150,

      2847,

      721,

      348,

      3525,

      2586,

      1082,

      1905,
      -1,

      2262,

      1442,

      2328,

      3519,

      3135,

      1415,

      1073,

      1896,

      3513,

      718,
      -1,

      68,

      1430,

      1490,

      733,

      2250,

      2259,

      2316,

      2325,

      3105,

      1403,

      1412,
      -1,

      3093,

      2556,

      1025,

      1850,

      1484,

      727,

      303,

      3099,

      3321,

      2481,

      950,

      3309,

      2469,

      938,

      267,

      3258,

      3246,

      261,

      1781,

      1775,

      2184,

      240,

      234,

      2172,

      3306,

      2466,

      935,

      1805,

      3264,

      2424,

      893,

      1784,

      1331,

      1325,
      -1,

      3060,

      2613,

      1097,

      1932,

      1739,

      2151,

      3168,

      198,

      1736,

      2559,

      1028,

      195,
      -1,

      2115,
      -1,

      2994,

      2988,

      2109,

      1289,

      3279,

      2439,

      908,

      1286,

      1721,

      3015,

      1718,

      180,

      1715,

      177,

      2802,

      174,

      2166,

      2103,

      2796,

      2097,

      3117,

      2091,

      1271,
      -1,

      1268,

      3,

      1265,
      -1,

      1640,

      2208,

      2124,

      3453,

      77,

      2766,

      1337,

      2760,

      2280,

      2754,

      1238,

      2058,

      1691,
      -1,

      2118,

      3504,

      129,

      1628,

      2841,

      450,

      3441,
      -1,

      2568,

      1622,

      2940,

      447,

      3435,

      583,

      2391,

      15,

      2805,

      568,

      562,

      2220,

      1616,

      2934,

      444,

      3429,

      1349,

      1610,

      580,

      441,

      3423,

      2112,

      571,

      2379,

      1604,

      1757,

      438,

      3417,

      216,

      2373,

      529,

      1079,

      1902,

      1598,

      526,

      435,

      3411,

      2799,

      49,

      1307,

      2367,

      2907,

      2232,

      2337,

      2298,

      2361,

      1424,

      1385,
      -1,

      511,

      2838,

      508,

      2355,

      505,

      1175,
      -1,

      1568,

      817,
      -1,

      3381,

      1169,

      2349,

      1556,

      804,

      3096,

      3369,

      2673,

      1157,

      1977,

      1529,

      776,

      2661,

      1145,

      1965,

      1505,

      746,

      2961,

      3594,

      2655,

      1139,

      1959,

      1493,

      736,

      3588,

      2646,

      1130,

      1950,

      1475,

      2643,

      1127,

      1947,

      1469,
      -1,

      646,

      3582,

      2610,

      1094,

      1929,

      2595,

      3576,

      1914,

      2583,
      -1,

      1031,

      1853,
      -1,

      3570,

      306,

      62,

      2607,

      1091,

      1926,

      2244,
      -1,

      2310,

      3564,

      547,

      1397,

      3147,

      3333,

      2493,

      962,

      3144,

      1754,

      667,

      273,

      213,

      2625,

      1109,

      3114,
      -1, -1,

      3108,

      3327,

      2487,

      956,

      3063,

      1304,

      3534,

      270,

      3300,

      2460,

      929,

      1802,

      3522,

      3297,

      2457,

      926,

      2832,
      -1, -1,

      255,

      3180,

      3294,

      2454,

      923,

      1799,

      3288,

      2448,

      917,

      1796,

      3126,

      1748,

      2100,

      1745,

      207,

      3174,

      204,

      1733,

      1730,

      1727,

      192,

      189,

      186,

      2736,

      3012,

      1298,

      1676,

      1295,

      2763,

      3489,

      114,

      1283,

      1280,

      1277,
      -1, -1,

      2820,

      3009,

      2814,
      -1,

      682,

      3006,

      2790,

      2784,

      2778,

      1250,

      2070,

      2628,

      1112,

      490,

      1658,

      150,

      1652,

      3471,

      96,

      3465,

      89,

      1646,

      2619,

      1103,

      3459,

      83,

      1634,

      544,
      -1,

      3447,

      2649,

      1133,

      1953,

      1481,

      2604,

      1088,

      1923,

      2598,
      -1,

      1917,
      -1, -1,

      2409,

      2571,

      2403,

      3129,

      2535,

      1004,

      2958,

      2397,

      21,

      2943,

      577,

      2094,

      2385,

      2910,

      3120,

      2529,

      998,

      1355,
      -1,

      56,

      2955,

      2523,

      992,

      2238,

      538,

      2304,

      535,

      2757,

      1391,
      -1,

      523,

      520,

      517,

      43,
      -1, -1,

      3222,

      2226,

      2973,

      2292,

      2517,

      986,

      1379,

      3345,

      2505,

      974,
      -1,

      3216,

      3339,

      2499,

      968,
      -1, -1,

      3210,

      3291,

      2451,

      920,

      3273,

      2433,

      902,

      252,
      -1,

      703,

      3267,

      2427,

      896,

      3600,
      -1, -1,

      2160,
      -1, -1,

      3204,

      1709,
      -1,

      2154,

      3192,

      3261,

      2421,

      890,
      -1,

      3186,

      1508,

      749,

      402,

      351,

      1706,
      -1,

      688,

      2148,

      3285,

      2445,

      914,

      2334,

      649,
      -1,

      1421,

      619,
      -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1,

      616,
      -1, -1, -1, -1, -1,

      610,
      -1, -1, -1, -1, -1,

      2967,
      -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,

      2952,

      604,
      -1,

      2946
    };

  if (0 == 0)
    {
      int key = hash(codes);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          int index = wordlist[key];

          if (index >= 0 && onig_codes_cmp(codes, OnigUnicodeFolds1 + index, 1) == 0)
            return index;
        }
    }
  return -1;
}


