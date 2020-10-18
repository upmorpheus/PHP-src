/*
 * "streamable kanji code filter and converter"
 * Copyright (c) 1998-2002 HappySize, Inc. All rights reserved.
 *
 * LICENSE NOTICES
 *
 * This file is part of "streamable kanji code filter and converter",
 * which is distributed under the terms of GNU Lesser General Public
 * License (version 2) as published by the Free Software Foundation.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with "streamable kanji code filter and converter";
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 *
 * The author of this file: Hayk Chamyan <hamshen@gmail.com>
 *
 */

#ifndef UNICODE_TABLE_ARMSCII8_H
#define UNICODE_TABLE_ARMSCII8_H

/* ArmSCII-8 to Unicode table */
static const unsigned short armscii8_ucs_table[] = {
0x00a0, 0x0000, 0x0587, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab,
0x2014, 0x002e, 0x055d, 0x002c, 0x002d, 0x058a, 0x2026, 0x055c,
0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563,
0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567,
0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b,
0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f,
0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573,
0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577,
0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b,
0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f,
0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583,
0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x055a, 0x0000
};

static const unsigned char ucs_armscii8_table[] = {
0xa5, 0xa4, 0x2a, 0x2b, 0xab, 0xac, 0xa9, 0x2f
};

static const int armscii8_ucs_table_min = 0xa0;
static const int armscii8_ucs_table_len = (sizeof (armscii8_ucs_table) / sizeof (unsigned short));
static const int armscii8_ucs_table_max = 0xa0 + (sizeof (armscii8_ucs_table) / sizeof (unsigned short));

#endif /* UNICODE_TABLE_ARMSCII8_H */
