/*
 * Copyright (C) 2023 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

/*
 * Caution!
 * This file generated by the script "utils/lexbor/css/names.py"!
 * Do not change this file!
 */


#ifndef LXB_CSS_AT_RULE_CONST_H
#define LXB_CSS_AT_RULE_CONST_H


#include "lexbor/css/value/const.h"


enum {
    LXB_CSS_AT_RULE__UNDEF      = 0x0000,
    LXB_CSS_AT_RULE__CUSTOM     = 0x0001,
    LXB_CSS_AT_RULE_MEDIA       = 0x0002,
    LXB_CSS_AT_RULE_NAMESPACE   = 0x0003,
    LXB_CSS_AT_RULE__LAST_ENTRY = 0x0004
};
typedef uintptr_t lxb_css_at_rule_type_t;


#endif /* LXB_CSS_AT_RULE_CONST_H */
