/**********************************************************************

  iso8859_8.c -  Oniguruma (regular expression library)

  Copyright (C) 2004  K.Kosako (kosako@sofnec.co.jp)

**********************************************************************/
#include "regenc.h"

#define ENC_IS_ISO_8859_8_CTYPE(code,ctype) \
  ((EncISO_8859_8_CtypeTable[code] & ctype) != 0)

static unsigned short EncISO_8859_8_CtypeTable[256] = {
  0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004,
  0x1004, 0x1106, 0x1104, 0x1104, 0x1104, 0x1104, 0x1004, 0x1004,
  0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004,
  0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004,
  0x1142, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0,
  0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0,
  0x1c58, 0x1c58, 0x1c58, 0x1c58, 0x1c58, 0x1c58, 0x1c58, 0x1c58,
  0x1c58, 0x1c58, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x10d0,
  0x10d0, 0x1e51, 0x1e51, 0x1e51, 0x1e51, 0x1e51, 0x1e51, 0x1a51,
  0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51,
  0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51, 0x1a51,
  0x1a51, 0x1a51, 0x1a51, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x18d0,
  0x10d0, 0x1c71, 0x1c71, 0x1c71, 0x1c71, 0x1c71, 0x1c71, 0x1871,
  0x1871, 0x1871, 0x1871, 0x1871, 0x1871, 0x1871, 0x1871, 0x1871,
  0x1871, 0x1871, 0x1871, 0x1871, 0x1871, 0x1871, 0x1871, 0x1871,
  0x1871, 0x1871, 0x1871, 0x10d0, 0x10d0, 0x10d0, 0x10d0, 0x1004,
  0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
  0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
  0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
  0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
  0x0142, 0x0000, 0x0050, 0x0050, 0x0050, 0x0050, 0x0050, 0x0050,
  0x0050, 0x0050, 0x0050, 0x00d0, 0x0050, 0x00d0, 0x0050, 0x0050,
  0x0050, 0x0050, 0x0850, 0x0850, 0x0050, 0x0871, 0x0050, 0x00d0,
  0x0050, 0x0850, 0x0050, 0x00d0, 0x0850, 0x0850, 0x0850, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d0,
  0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851,
  0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851,
  0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851, 0x0851,
  0x0851, 0x0851, 0x0851, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static int
iso_8859_8_code_is_ctype(OnigCodePoint code, unsigned int ctype)
{
  if (code < 256)
    return ENC_IS_ISO_8859_8_CTYPE(code, ctype);
  else
    return FALSE;
}

OnigEncodingType OnigEncodingISO_8859_8 = {
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
  },
  "ISO-8859-8",  /* name */
  1,             /* max byte length */
  FALSE,         /* is_fold_match */
  ONIGENC_CTYPE_SUPPORT_LEVEL_SB,    /* ctype_support_level */
  TRUE,          /* is continuous sb mb codepoint */
  onigenc_single_byte_mbc_to_code,
  onigenc_single_byte_code_to_mbclen,
  onigenc_single_byte_code_to_mbc,
  onigenc_ascii_mbc_to_lower,
  onigenc_ascii_mbc_is_case_ambig,
  iso_8859_8_code_is_ctype,
  onigenc_nothing_get_ctype_code_range,
  onigenc_single_byte_left_adjust_char_head,
  onigenc_single_byte_is_allowed_reverse_match,
  onigenc_nothing_get_all_fold_match_code,
  onigenc_nothing_get_fold_match_info
};
