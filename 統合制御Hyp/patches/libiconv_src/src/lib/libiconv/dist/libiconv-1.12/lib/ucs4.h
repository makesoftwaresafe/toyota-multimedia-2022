/*
 * Copyright (C) 1999-2001 Free Software Foundation, Inc.
 * This file is part of the GNU LIBICONV Library.
 *
 * The GNU LIBICONV Library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * The GNU LIBICONV Library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU LIBICONV Library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * UCS-4
 */

/* Here we accept FFFE0000/0000FEFF marks as endianness indicators everywhere
   in the stream, not just at the beginning. The default is big-endian. */
/* The state is 0 if big-endian, 1 if little-endian. */
static int
ucs4_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  state_t state = conv->istate;
  int count = 0;
  for (; n >= 4;) {
    ucs4_t wc = (state
                  ? s[0] + (s[1] << 8) + (s[2] << 16) + (s[3] << 24)
                  : (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3]);
    s += 4; n -= 4; count += 4;
    if (wc == 0x0000feff) {
    } else if (wc == 0xfffe0000u) {
      state ^= 1;
    } else if (wc <= 0x7fffffff) {
      *pwc = wc;
      conv->istate = state;
      return count;
    } else
      return RET_ILSEQ;
  }
  conv->istate = state;
  return RET_TOOFEW(count);
}

/* But we output UCS-4 in big-endian order, without byte-order mark. */
static int
ucs4_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc <= 0x7fffffff) {
    if (n >= 4) {
      r[0] = (unsigned char) (wc >> 24);
      r[1] = (unsigned char) (wc >> 16);
      r[2] = (unsigned char) (wc >> 8);
      r[3] = (unsigned char) wc;
      return 4;
    } else
      return RET_TOOSMALL;
  } else
    return RET_ILUNI;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/lib/libiconv/dist/libiconv-1.12/lib/ucs4.h $ $Rev: 680336 $")
#endif
