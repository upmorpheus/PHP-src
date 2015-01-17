/* UBidiPairedBracketType - http://icu-project.org/apiref/icu4c/uchar_8h.html#af954219aa1df452657ec355221c6703d */
#if U_ICU_VERSION_MAJOR_NUM >= 52
UOTHER(BPT_NONE)
UOTHER(BPT_OPEN)
UOTHER(BPT_CLOSE)
UOTHER(BPT_COUNT)
#endif /* ICU >= 52 */

/* UEastAsianWidth - http://icu-project.org/apiref/icu4c/uchar_8h.html#a95cc2ca2f9cfd6d0c63eee2c65951333 */
UOTHER(EA_NEUTRAL)
UOTHER(EA_AMBIGUOUS)
UOTHER(EA_HALFWIDTH)
UOTHER(EA_FULLWIDTH )
UOTHER(EA_NARROW)
UOTHER(EA_WIDE)
UOTHER(EA_COUNT)

/* UCharNameChoice - http://icu-project.org/apiref/icu4c/uchar_8h.html#a2ba37edcca62eff48226e8096035addf */
UOTHER(UNICODE_CHAR_NAME)
UOTHER(UNICODE_10_CHAR_NAME)
UOTHER(EXTENDED_CHAR_NAME)
#if U_ICU_VERSION_MAJOR_NUM * 10 + U_ICU_VERSION_MINOR_NUM >= 44
UOTHER(CHAR_NAME_ALIAS)
#endif /* ICU >= 4.4 */
UOTHER(CHAR_NAME_CHOICE_COUNT)

/* UPropertyNameChoice - http://icu-project.org/apiref/icu4c/uchar_8h.html#a5056494c7d5a2c7185f3c464f48fe5d1 */
UOTHER(SHORT_PROPERTY_NAME)
UOTHER(LONG_PROPERTY_NAME)
UOTHER(PROPERTY_NAME_CHOICE_COUNT)

/* UDecompositionType - http://icu-project.org/apiref/icu4c/uchar_8h.html#ae2c56994fcf28062c7e77beb671533f5 */
UOTHER(DT_NONE)
UOTHER(DT_CANONICAL)
UOTHER(DT_COMPAT)
UOTHER(DT_CIRCLE)
UOTHER(DT_FINAL)
UOTHER(DT_FONT)
UOTHER(DT_FRACTION)
UOTHER(DT_INITIAL)
UOTHER(DT_ISOLATED)
UOTHER(DT_MEDIAL)
UOTHER(DT_NARROW)
UOTHER(DT_NOBREAK)
UOTHER(DT_SMALL)
UOTHER(DT_SQUARE)
UOTHER(DT_SUB)
UOTHER(DT_SUPER)
UOTHER(DT_VERTICAL)
UOTHER(DT_WIDE)
UOTHER(DT_COUNT )

/* UJoiningType - http://icu-project.org/apiref/icu4c/uchar_8h.html#a3ce1ce20e7f3b8534eb3490ad3aba3dd */
UOTHER(JT_NON_JOINING)
UOTHER(JT_JOIN_CAUSING)
UOTHER(JT_DUAL_JOINING)
UOTHER(JT_LEFT_JOINING)
UOTHER(JT_RIGHT_JOINING)
UOTHER(JT_TRANSPARENT)
UOTHER(JT_COUNT)

/* UJoiningGroup - http://icu-project.org/apiref/icu4c/uchar_8h.html#a7887844ec0872e6e9a283e0825fcae65 */
UOTHER(JG_NO_JOINING_GROUP)
UOTHER(JG_AIN)
UOTHER(JG_ALAPH)
UOTHER(JG_ALEF)
UOTHER(JG_BEH)
UOTHER(JG_BETH)
UOTHER(JG_DAL)
UOTHER(JG_DALATH_RISH)
UOTHER(JG_E)
UOTHER(JG_FEH)
UOTHER(JG_FINAL_SEMKATH)
UOTHER(JG_GAF)
UOTHER(JG_GAMAL)
UOTHER(JG_HAH)
#if U_ICU_VERSION_MAJOR_NUM * 10 + U_ICU_VERSION_MINOR_NUM >= 46
UOTHER(JG_TEH_MARBUTA_GOAL)
#endif /* ICU >= 4.6 */
UOTHER(JG_HAMZA_ON_HEH_GOAL)
UOTHER(JG_HE)
UOTHER(JG_HEH)
UOTHER(JG_HEH_GOAL)
UOTHER(JG_HETH)
UOTHER(JG_KAF)
UOTHER(JG_KAPH)
UOTHER(JG_KNOTTED_HEH)
UOTHER(JG_LAM)
UOTHER(JG_LAMADH)
UOTHER(JG_MEEM)
UOTHER(JG_MIM)
UOTHER(JG_NOON)
UOTHER(JG_NUN)
UOTHER(JG_PE)
UOTHER(JG_QAF)
UOTHER(JG_QAPH)
UOTHER(JG_REH)
UOTHER(JG_REVERSED_PE)
UOTHER(JG_SAD)
UOTHER(JG_SADHE)
UOTHER(JG_SEEN)
UOTHER(JG_SEMKATH)
UOTHER(JG_SHIN)
UOTHER(JG_SWASH_KAF)
UOTHER(JG_SYRIAC_WAW)
UOTHER(JG_TAH)
UOTHER(JG_TAW)
UOTHER(JG_TEH_MARBUTA)
UOTHER(JG_TETH)
UOTHER(JG_WAW)
UOTHER(JG_YEH)
UOTHER(JG_YEH_BARREE)
UOTHER(JG_YEH_WITH_TAIL)
UOTHER(JG_YUDH)
UOTHER(JG_YUDH_HE)
UOTHER(JG_ZAIN)
UOTHER(JG_FE)
UOTHER(JG_KHAPH)
UOTHER(JG_ZHAIN)
UOTHER(JG_BURUSHASKI_YEH_BARREE)
#if U_ICU_VERSION_MAJOR_NUM * 10 + U_ICU_VERSION_MINOR_NUM >= 44
UOTHER(JG_FARSI_YEH)
UOTHER(JG_NYA)
#endif /* ICU >= 4.4 */
#if U_ICU_VERSION_MAJOR_NUM >= 49
UOTHER(JG_ROHINGYA_YEH)
#endif
#if U_ICU_VERSION_MAJOR_NUM >= 54
UOTHER(JG_MANICHAEAN_ALEPH)
UOTHER(JG_MANICHAEAN_AYIN)
UOTHER(JG_MANICHAEAN_BETH)
UOTHER(JG_MANICHAEAN_DALETH)
UOTHER(JG_MANICHAEAN_DHAMEDH)
UOTHER(JG_MANICHAEAN_FIVE)
UOTHER(JG_MANICHAEAN_GIMEL)
UOTHER(JG_MANICHAEAN_HETH)
UOTHER(JG_MANICHAEAN_HUNDRED)
UOTHER(JG_MANICHAEAN_KAPH)
UOTHER(JG_MANICHAEAN_LAMEDH)
UOTHER(JG_MANICHAEAN_MEM)
UOTHER(JG_MANICHAEAN_NUN)
UOTHER(JG_MANICHAEAN_ONE)
UOTHER(JG_MANICHAEAN_PE)
UOTHER(JG_MANICHAEAN_QOPH)
UOTHER(JG_MANICHAEAN_RESH)
UOTHER(JG_MANICHAEAN_SADHE)
UOTHER(JG_MANICHAEAN_SAMEKH)
UOTHER(JG_MANICHAEAN_TAW)
UOTHER(JG_MANICHAEAN_TEN)
UOTHER(JG_MANICHAEAN_TETH)
UOTHER(JG_MANICHAEAN_THAMEDH)
UOTHER(JG_MANICHAEAN_TWENTY)
UOTHER(JG_MANICHAEAN_WAW)
UOTHER(JG_MANICHAEAN_YODH)
UOTHER(JG_MANICHAEAN_ZAYIN)
UOTHER(JG_STRAIGHT_WAW)
#endif /* ICU 54 */
UOTHER(JG_COUNT )

/* UGraphemeClusterBreak - http://icu-project.org/apiref/icu4c/uchar_8h.html#abb9bae7d2a1c80ce342be4647661fde1 */
UOTHER(GCB_OTHER)
UOTHER(GCB_CONTROL)
UOTHER(GCB_CR)
UOTHER(GCB_EXTEND)
UOTHER(GCB_L)
UOTHER(GCB_LF)
UOTHER(GCB_LV)
UOTHER(GCB_LVT)
UOTHER(GCB_T)
UOTHER(GCB_V)
UOTHER(GCB_SPACING_MARK)
UOTHER(GCB_PREPEND)
#if U_ICU_VERSION_MAJOR_NUM >= 50
UOTHER(GCB_REGIONAL_INDICATOR)
#endif /* ICU 50 */
UOTHER(GCB_COUNT)

/* UWordBreakValues - http://icu-project.org/apiref/icu4c/uchar_8h.html#af70ee907368e663f8dd4b90c7196e15c */
UOTHER(WB_OTHER)
UOTHER(WB_ALETTER)
UOTHER(WB_FORMAT)
UOTHER(WB_KATAKANA)
UOTHER(WB_MIDLETTER)
UOTHER(WB_MIDNUM)
UOTHER(WB_NUMERIC)
UOTHER(WB_EXTENDNUMLET)
UOTHER(WB_CR)
UOTHER(WB_EXTEND)
UOTHER(WB_LF)
UOTHER(WB_MIDNUMLET)
UOTHER(WB_NEWLINE)
#if U_ICU_VERSION_MAJOR_NUM >= 50
UOTHER(WB_REGIONAL_INDICATOR)
#endif /* ICU >= 50 */
#if U_ICU_VERSION_MAJOR_NUM >= 52
UOTHER(WB_HEBREW_LETTER)
UOTHER(WB_SINGLE_QUOTE)
UOTHER(WB_DOUBLE_QUOTE)
#endif /* ICU >= 52 */
UOTHER(WB_COUNT)

/* USentenceBreak - http://icu-project.org/apiref/icu4c/uchar_8h.html#a89e9e463c3bae1d2d46b1dbb6f90de0f */
UOTHER(SB_OTHER)
UOTHER(SB_ATERM)
UOTHER(SB_CLOSE)
UOTHER(SB_FORMAT)
UOTHER(SB_LOWER)
UOTHER(SB_NUMERIC)
UOTHER(SB_OLETTER)
UOTHER(SB_SEP)
UOTHER(SB_SP)
UOTHER(SB_STERM)
UOTHER(SB_UPPER)
UOTHER(SB_CR)
UOTHER(SB_EXTEND)
UOTHER(SB_LF)
UOTHER(SB_SCONTINUE)
UOTHER(SB_COUNT)

/* ULineBreak - http://icu-project.org/apiref/icu4c/uchar_8h.html#a5d1abdf05be22cb9599f804a8506277c */
UOTHER(LB_UNKNOWN)
UOTHER(LB_AMBIGUOUS)
UOTHER(LB_ALPHABETIC)
UOTHER(LB_BREAK_BOTH)
UOTHER(LB_BREAK_AFTER)
UOTHER(LB_BREAK_BEFORE)
UOTHER(LB_MANDATORY_BREAK)
UOTHER(LB_CONTINGENT_BREAK)
UOTHER(LB_CLOSE_PUNCTUATION)
UOTHER(LB_COMBINING_MARK)
UOTHER(LB_CARRIAGE_RETURN)
UOTHER(LB_EXCLAMATION)
UOTHER(LB_GLUE)
UOTHER(LB_HYPHEN)
UOTHER(LB_IDEOGRAPHIC)
UOTHER(LB_INSEPARABLE)
UOTHER(LB_INSEPERABLE)
UOTHER(LB_INFIX_NUMERIC)
UOTHER(LB_LINE_FEED)
UOTHER(LB_NONSTARTER)
UOTHER(LB_NUMERIC)
UOTHER(LB_OPEN_PUNCTUATION)
UOTHER(LB_POSTFIX_NUMERIC)
UOTHER(LB_PREFIX_NUMERIC)
UOTHER(LB_QUOTATION)
UOTHER(LB_COMPLEX_CONTEXT)
UOTHER(LB_SURROGATE)
UOTHER(LB_SPACE)
UOTHER(LB_BREAK_SYMBOLS)
UOTHER(LB_ZWSPACE)
UOTHER(LB_NEXT_LINE)
UOTHER(LB_WORD_JOINER)
UOTHER(LB_H2)
UOTHER(LB_H3)
UOTHER(LB_JL)
UOTHER(LB_JT)
UOTHER(LB_JV)
#if U_ICU_VERSION_MAJOR_NUM * 10 + U_ICU_VERSION_MINOR_NUM >= 44
UOTHER(LB_CLOSE_PARENTHESIS)
#endif /* ICU >= 4.4 */
#if U_ICU_VERSION_MAJOR_NUM >= 49
UOTHER(LB_CONDITIONAL_JAPANESE_STARTER)
UOTHER(LB_HEBREW_LETTER)
#endif /* ICU >= 49 */
#if U_ICU_VERSION_MAJOR_NUM >= 50
UOTHER(LB_REGIONAL_INDICATOR)
#endif /* ICU >= 50 */
UOTHER(LB_COUNT)

/* UNumericType - http://icu-project.org/apiref/icu4c/uchar_8h.html#adec3e7a6ae3a00274c019b3b2ddaecbe */
UOTHER(NT_NONE)
UOTHER(NT_DECIMAL)
UOTHER(NT_DIGIT)
UOTHER(NT_NUMERIC)
UOTHER(NT_COUNT)

/* UHangulSyllableType - http://icu-project.org/apiref/icu4c/uchar_8h.html#a7cb09027c37ad73571cf541caf002c8f */
UOTHER(HST_NOT_APPLICABLE)
UOTHER(HST_LEADING_JAMO)
UOTHER(HST_VOWEL_JAMO)
UOTHER(HST_TRAILING_JAMO)
UOTHER(HST_LV_SYLLABLE)
UOTHER(HST_LVT_SYLLABLE)
UOTHER(HST_COUNT )
