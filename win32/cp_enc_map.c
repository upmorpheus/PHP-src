/* Autogenerated file. Update cp_enc_map_gen.c and regen like 
 cp_enc_map_gen.exe > cp_enc_map.c 
*/

static const struct php_win32_cp php_win32_cp_map[] = {	{ 37, 0, 0, 1, "IBM037", NULL, "37    (IBM EBCDIC - U.S./Canada)" },
	{ 437, 0, 0, 1, "IBM437", NULL, "437   (OEM - United States)" },
	{ 500, 0, 0, 1, "IBM500", NULL, "500   (IBM EBCDIC - International)" },
	{ 708, 0, 0, 1, "ASMO-708", NULL, "708   (Arabic - ASMO)" },
	/* 709 is invalid */
	/* 710 is invalid */
	{ 720, 0, 0, 1, "DOS-720", NULL, "720   (Arabic - Transparent ASMO)" },
	{ 737, 0, 0, 1, "ibm737", NULL, "737   (OEM - Greek 437G)" },
	{ 775, 0, 0, 1, "ibm775", NULL, "775   (OEM - Baltic)" },
	{ 850, 0, 0, 1, "ibm850", "850|CP850|IBM850|CSPC850MULTILINGUAL", "850   (OEM - Multilingual Latin I)" },
	{ 852, 0, 0, 1, "ibm852", NULL, "852   (OEM - Latin II)" },
	{ 855, 0, 0, 1, "IBM855", NULL, "855   (OEM - Cyrillic)" },
	{ 857, 0, 0, 1, "ibm857", NULL, "857   (OEM - Turkish)" },
	{ 858, 0, 0, 1, "IBM00858", NULL, "858   (OEM - Multilingual Latin I + Euro)" },
	{ 860, 0, 0, 1, "IBM860", NULL, "860   (OEM - Portuguese)" },
	{ 861, 0, 0, 1, "ibm861", NULL, "861   (OEM - Icelandic)" },
	{ 862, 0, 0, 1, "DOS-862", "862|CP862|IBM862|CSPC862LATINHEBREW", "862   (OEM - Hebrew)" },
	{ 863, 0, 0, 1, "IBM863", NULL, "863   (OEM - Canadian French)" },
	{ 864, 0, 0, 1, "IBM864", NULL, "864   (OEM - Arabic)" },
	{ 865, 0, 0, 1, "IBM865", NULL, "865   (OEM - Nordic)" },
	{ 866, 0, 0, 1, "cp866", "866|CP866|IBM866|CSIBM866", "866   (OEM - Russian)" },
	{ 869, 0, 0, 1, "ibm869", NULL, "869   (OEM - Modern Greek)" },
	{ 870, 0, 0, 1, "IBM870", NULL, "870   (IBM EBCDIC - Multilingual/ROECE (Latin-2))" },
	{ 874, 0, 0, 1, "windows-874", "CP874", "874   (ANSI/OEM - Thai)" },
	{ 875, 0, 0, 1, "cp875", NULL, "875   (IBM EBCDIC - Modern Greek)" },
	{ 932, 0, 0, 2, "shift_jis", "CP932|SHIFT_JIS|MS_KANJI|CSSHIFTJIS", "932   (ANSI/OEM - Japanese Shift-JIS)" },
	{ 936, 0, 0, 2, "gb2312", "GB2312|GBK|CP936|MS936|WINDOWS-936", "936   (ANSI/OEM - Simplified Chinese GBK)" },
	{ 949, 0, 0, 2, "ks_c_5601-1987", "CP949|UHC", "949   (ANSI/OEM - Korean)" },
	{ 950, 0, 0, 2, "big5", "CP950|BIG-5", "950   (ANSI/OEM - Traditional Chinese Big5)" },
	{ 1026, 0, 0, 1, "IBM1026", NULL, "1026  (IBM EBCDIC - Turkish (Latin-5))" },
	{ 1047, 0, 0, 1, "IBM01047", NULL, "1047  (IBM EBCDIC - Latin-1/Open System)" },
	{ 1140, 0, 0, 1, "IBM01140", NULL, "1140  (IBM EBCDIC - U.S./Canada (37 + Euro))" },
	{ 1141, 0, 0, 1, "IBM01141", NULL, "1141  (IBM EBCDIC - Germany (20273 + Euro))" },
	{ 1142, 0, 0, 1, "IBM01142", NULL, "1142  (IBM EBCDIC - Denmark/Norway (20277 + Euro))" },
	{ 1143, 0, 0, 1, "IBM01143", NULL, "1143  (IBM EBCDIC - Finland/Sweden (20278 + Euro))" },
	{ 1144, 0, 0, 1, "IBM01144", NULL, "1144  (IBM EBCDIC - Italy (20280 + Euro))" },
	{ 1145, 0, 0, 1, "IBM01145", NULL, "1145  (IBM EBCDIC - Latin America/Spain (20284 + Euro))" },
	{ 1146, 0, 0, 1, "IBM01146", NULL, "1146  (IBM EBCDIC - United Kingdom (20285 + Euro))" },
	{ 1148, 0, 0, 1, "IBM01148", NULL, "1148  (IBM EBCDIC - International (500 + Euro))" },
	{ 1149, 0, 0, 1, "IBM01149", NULL, "1149  (IBM EBCDIC - Icelandic (20871 + Euro))" },
	/* 1200 is invalid */
	/* 1201 is invalid */
	{ 1250, 0, 0, 1, "windows-1250", "CP1250|MS-EE|WINDOWS-1250", "1250  (ANSI - Central Europe)" },
	{ 1251, 0, 0, 1, "windows-1251", "CP1251|MS-CYRL|WINDOWS-1251", "1251  (ANSI - Cyrillic)" },
	{ 1252, 0, 0, 1, "windows-1252", "CP1252|MS-ANSI|WINDOWS-1252", "1252  (ANSI - Latin I)" },
	{ 1253, 0, 0, 1, "windows-1253", "CP1253|MS-GREEK|WINDOWS-1253", "1253  (ANSI - Greek)" },
	{ 1254, 0, 0, 1, "windows-1254", "CP1254|MS-TURK|WINDOWS-1254", "1254  (ANSI - Turkish)" },
	{ 1255, 0, 0, 1, "windows-1255", "CP1255|MS-HEBR|WINDOWS-1255", "1255  (ANSI - Hebrew)" },
	{ 1256, 0, 0, 1, "windows-1256", "CP1256|MS-ARAB|WINDOWS-1256", "1256  (ANSI - Arabic)" },
	{ 1257, 0, 0, 1, "windows-1257", "CP1257|WINBALTRIM|WINDOWS-1257", "1257  (ANSI - Baltic)" },
	{ 1258, 0, 0, 1, "windows-1258", "CP1258|WINDOWS-1258", "1258  (ANSI/OEM - Viet Nam)" },
	{ 1361, 0, 0, 2, "Johab", "CP1361|JOHAB", "1361  (Korean - Johab)" },
	{ 10000, 0, 0, 1, "macintosh", "MAC|MACINTOSH|MACROMAN|CSMACINTOSH", "10000 (MAC - Roman)" },
	{ 10001, 0, 0, 2, "x-mac-japanese", NULL, "10001 (MAC - Japanese)" },
	{ 10002, 0, 0, 2, "x-mac-chinesetrad", NULL, "10002 (MAC - Traditional Chinese Big5)" },
	{ 10003, 0, 0, 2, "x-mac-korean", NULL, "10003 (MAC - Korean)" },
	{ 10004, 0, 0, 1, "x-mac-arabic", "MACARABIC", "10004 (MAC - Arabic)" },
	{ 10005, 0, 0, 1, "x-mac-hebrew", "MACHEBREW", "10005 (MAC - Hebrew)" },
	{ 10006, 0, 0, 1, "x-mac-greek", "MACGREEK", "10006 (MAC - Greek I)" },
	{ 10007, 0, 0, 1, "x-mac-cyrillic", "MACCYRILLIC", "10007 (MAC - Cyrillic)" },
	{ 10008, 0, 0, 2, "x-mac-chinesesimp", NULL, "10008 (MAC - Simplified Chinese GB 2312)" },
	{ 10010, 0, 0, 1, "x-mac-romanian", "MACROMANIA", "10010 (MAC - Romania)" },
	{ 10017, 0, 0, 1, "x-mac-ukrainian", "MACUKRAINE", "10017 (MAC - Ukraine)" },
	{ 10021, 0, 0, 1, "x-mac-thai", "MACTHAI", "10021 (MAC - Thai)" },
	{ 10029, 0, 0, 1, "x-mac-ce", "MACCENTRALEUROPE", "10029 (MAC - Latin II)" },
	{ 10079, 0, 0, 1, "x-mac-icelandic", "MACICELAND", "10079 (MAC - Icelandic)" },
	{ 10081, 0, 0, 1, "x-mac-turkish", "MACTURKISH", "10081 (MAC - Turkish)" },
	{ 10082, 0, 0, 1, "x-mac-croatian", "MACCROATIAN", "10082 (MAC - Croatia)" },
	/* 12000 is invalid */
	/* 12001 is invalid */
	{ 20000, 0, 0, 2, "x-Chinese_CNS", NULL, "20000 (CNS - Taiwan)" },
	{ 20001, 0, 0, 2, "x-cp20001", NULL, "20001 (TCA - Taiwan)" },
	{ 20002, 0, 0, 2, "x_Chinese-Eten", NULL, "20002 (Eten - Taiwan)" },
	{ 20003, 0, 0, 2, "x-cp20003", NULL, "20003 (IBM5550 - Taiwan)" },
	{ 20004, 0, 0, 2, "x-cp20004", NULL, "20004 (TeleText - Taiwan)" },
	{ 20005, 0, 0, 2, "x-cp20005", NULL, "20005 (Wang - Taiwan)" },
	{ 20105, 0, 0, 1, "x-IA5", NULL, "20105 (IA5 IRV International Alphabet No.5)" },
	{ 20106, 0, 0, 1, "x-IA5-German", NULL, "20106 (IA5 German)" },
	{ 20107, 0, 0, 1, "x-IA5-Swedish", NULL, "20107 (IA5 Swedish)" },
	{ 20108, 0, 0, 1, "x-IA5-Norwegian", NULL, "20108 (IA5 Norwegian)" },
	{ 20127, 0, 0, 1, "us-ascii", NULL, "20127 (US-ASCII)" },
	{ 20261, 0, 0, 2, "x-cp20261", NULL, "20261 (T.61)" },
	{ 20269, 0, 0, 1, "x-cp20269", NULL, "20269 (ISO 6937 Non-Spacing Accent)" },
	{ 20273, 0, 0, 1, "IBM273", NULL, "20273 (IBM EBCDIC - Germany)" },
	{ 20277, 0, 0, 1, "IBM277", NULL, "20277 (IBM EBCDIC - Denmark/Norway)" },
	{ 20278, 0, 0, 1, "IBM278", NULL, "20278 (IBM EBCDIC - Finland/Sweden)" },
	{ 20280, 0, 0, 1, "IBM280", NULL, "20280 (IBM EBCDIC - Italy)" },
	{ 20284, 0, 0, 1, "IBM284", NULL, "20284 (IBM EBCDIC - Latin America/Spain)" },
	{ 20285, 0, 0, 1, "IBM285", NULL, "20285 (IBM EBCDIC - United Kingdom)" },
	{ 20290, 0, 0, 1, "IBM290", NULL, "20290 (IBM EBCDIC - Japanese Katakana Extended)" },
	{ 20297, 0, 0, 1, "IBM297", NULL, "20297 (IBM EBCDIC - France)" },
	{ 20420, 0, 0, 1, "IBM420", NULL, "20420 (IBM EBCDIC - Arabic)" },
	{ 20423, 0, 0, 1, "IBM423", NULL, "20423 (IBM EBCDIC - Greek)" },
	{ 20424, 0, 0, 1, "IBM424", NULL, "20424 (IBM EBCDIC - Hebrew)" },
	{ 20833, 0, 0, 1, "x-EBCDIC-KoreanExtended", NULL, "20833 (IBM EBCDIC - Korean Extended)" },
	{ 20838, 0, 0, 1, "IBM-Thai", NULL, "20838 (IBM EBCDIC - Thai)" },
	{ 20866, 0, 0, 1, "koi8-r", "KOI8-R|CSKOI8R", "20866 (Russian - KOI8)" },
	{ 20871, 0, 0, 1, "IBM871", NULL, "20871 (IBM EBCDIC - Icelandic)" },
	{ 20880, 0, 0, 1, "IBM880", NULL, "20880 (IBM EBCDIC - Cyrillic (Russian))" },
	{ 20905, 0, 0, 1, "IBM905", NULL, "20905 (IBM EBCDIC - Turkish)" },
	{ 20924, 0, 0, 1, "IBM00924", NULL, "20924 (IBM EBCDIC - Latin-1/Open System (1047 + Euro))" },
	{ 20932, 0, 0, 2, "EUC-JP", "EUC-JP|EUCJP|EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE|CSEUCPKDFMTJAPANESE", "20932 (JIS X 0208-1990 & 0212-1990)" },
	{ 20936, 0, 0, 2, "x-cp20936", NULL, "20936 (Simplified Chinese GB2312)" },
	{ 21025, 0, 0, 1, "cp1025", NULL, "21025 (IBM EBCDIC - Cyrillic (Serbian, Bulgarian))" },
	{ 21866, 0, 0, 1, "koi8-u", "KOI8-U", "21866 (Ukrainian - KOI8-U)" },
	{ 28591, 0, 0, 1, "iso-8859-1", "CP819|IBM819|ISO-8859-1|ISO-IR-100|ISO8859-1|ISO_8859-1|ISO_8859-1:1987|L1|LATIN1|CSISOLATIN1", "28591 (ISO 8859-1 Latin I)" },
	{ 28592, 0, 0, 1, "iso-8859-2", "ISO-8859-2|ISO-IR-101|ISO8859-2|ISO_8859-2|ISO_8859-2:1987|L2|LATIN2|CSISOLATIN2", "28592 (ISO 8859-2 Central Europe)" },
	{ 28593, 0, 0, 1, "iso-8859-3", "ISO-8859-3|ISO-IR-109|ISO8859-3|ISO_8859-3|ISO_8859-3:1988|L3|LATIN3|CSISOLATIN3", "28593 (ISO 8859-3 Latin 3)" },
	{ 28594, 0, 0, 1, "iso-8859-4", "ISO-8859-4|ISO-IR-110|ISO8859-4|ISO_8859-4|ISO_8859-4:1988|L4|LATIN4|CSISOLATIN4", "28594 (ISO 8859-4 Baltic)" },
	{ 28595, 0, 0, 1, "iso-8859-5", "CYRILLIC|ISO-8859-5|ISO-IR-144|ISO8859-5|ISO_8859-5|ISO_8859-5:1988|CSISOLATINCYRILLIC", "28595 (ISO 8859-5 Cyrillic)" },
	{ 28596, 0, 0, 1, "iso-8859-6", "ARABIC|ASMO-708|ECMA-114|ISO-8859-6|ISO-IR-127|ISO8859-6|ISO_8859-6|ISO_8859-6:1987|CSISOLATINARABIC", "28596 (ISO 8859-6 Arabic)" },
	{ 28597, 0, 0, 1, "iso-8859-7", "ECMA-118|ELOT_928|GREEK|GREEK8|ISO-8859-7|ISO-IR-126|ISO8859-7|ISO_8859-7|ISO_8859-7:1987|ISO_8859-7:2003|CSISOLATINGREEK", "28597 (ISO 8859-7 Greek)" },
	{ 28598, 0, 0, 1, "iso-8859-8", "HEBREW|ISO-8859-8|ISO-IR-138|ISO8859-8|ISO_8859-8|ISO_8859-8:1988|CSISOLATINHEBREW", "28598 (ISO 8859-8 Hebrew: Visual Ordering)" },
	{ 28599, 0, 0, 1, "iso-8859-9", "ISO-8859-9|ISO-IR-148|ISO8859-9|ISO_8859-9|ISO_8859-9:1989|L5|LATIN5|CSISOLATIN5", "28599 (ISO 8859-9 Latin 5)" },
	{ 28603, 0, 0, 1, "iso-8859-13", "ISO-8859-13|ISO-IR-179|ISO8859-13|ISO_8859-13|L7|LATIN7", "28603 (ISO 8859-13 Latin 7)" },
	{ 28605, 0, 0, 1, "iso-8859-15", "ISO-8859-15|ISO-IR-203|ISO8859-15|ISO_8859-15|ISO_8859-15:1998|LATIN-9", "28605 (ISO 8859-15 Latin 9)" },
	/* 29001 is invalid */
	{ 38598, 0, 0, 1, "iso-8859-8-i", NULL, "38598 (ISO 8859-8 Hebrew: Logical Ordering)" },
	{ 50220, 0, 0, 5, "iso-2022-jp", "CP50220", "50220 (ISO-2022 Japanese with no halfwidth Katakana)" },
	{ 50221, 0, 0, 5, "csISO2022JP", "CP50221", "50221 (ISO-2022 Japanese with halfwidth Katakana)" },
	{ 50222, 0, 0, 5, "iso-2022-jp", "ISO-2022-JP|CP50222", "50222 (ISO-2022 Japanese JIS X 0201-1989)" },
	{ 50225, 0, 0, 5, "iso-2022-kr", "ISO-2022-KR|CSISO2022KR", "50225 (ISO-2022 Korean)" },
	{ 50227, 0, 0, 5, "x-cp50227", NULL, "50227 (ISO-2022 Simplified Chinese)" },
	{ 50229, 0, 0, 5, "x-cp50229", NULL, "50229 (ISO-2022 Traditional Chinese)" },
	/* 50930 is invalid */
	/* 50931 is invalid */
	/* 50933 is invalid */
	/* 50935 is invalid */
	/* 50936 is invalid */
	/* 50937 is invalid */
	/* 50939 is invalid */
	/* 51932 is invalid */
	/* 51936 is invalid */
	{ 51949, 0, 0, 2, "euc-kr", "EUC-KR|EUCKR|CSEUCKR", "51949 (EUC-Korean)" },
	/* 51950 is invalid */
	{ 52936, 0, 0, 5, "hz-gb-2312", "HZ|HZ-GB-2312", "52936 (HZ-GB2312 Simplified Chinese)" },
	{ 54936, 8, 128, 4, "GB18030", "GB18030|CSGB18030", "54936 (GB18030 Simplified Chinese)" },
	{ 57002, 0, 0, 4, "x-iscii-de", NULL, "57002 (ISCII - Devanagari)" },
	{ 57003, 0, 0, 4, "x-iscii-be", NULL, "57003 (ISCII - Bengali)" },
	{ 57004, 0, 0, 4, "x-iscii-ta", NULL, "57004 (ISCII - Tamil)" },
	{ 57005, 0, 0, 4, "x-iscii-te", NULL, "57005 (ISCII - Telugu)" },
	{ 57006, 0, 0, 4, "x-iscii-as", NULL, "57006 (ISCII - Assamese)" },
	{ 57007, 0, 0, 4, "x-iscii-or", NULL, "57007 (ISCII - Odia (Oriya))" },
	{ 57008, 0, 0, 4, "x-iscii-ka", NULL, "57008 (ISCII - Kannada)" },
	{ 57009, 0, 0, 4, "x-iscii-ma", NULL, "57009 (ISCII - Malayalam)" },
	{ 57010, 0, 0, 4, "x-iscii-gu", NULL, "57010 (ISCII - Gujarati)" },
	{ 57011, 0, 0, 4, "x-iscii-pa", NULL, "57011 (ISCII - Punjabi (Gurmukhi))" },
	{ 65000, 0, 0, 5, "utf-7", "UTF-7", "65000 (UTF-7)" },
	{ 65001, 8, 128, 4, "utf-8", "UTF-8", "65001 (UTF-8)" },
};

