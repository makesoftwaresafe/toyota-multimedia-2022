/*
 * Copyright (C) 1999-2001, 2005, 2006 Free Software Foundation, Inc.
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
 * eucJP-ms
 */

#include "eucjp_msext.h"

static int
eucjp_ms_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c = *s;
  /* Code set 0 (ASCII or JIS X 0201-1976 Roman) */
  if (c < 0x80)
    return ascii_mbtowc(conv,pwc,s,n);
  /* Code set 1 (JIS X 0208) */
  if (c >= 0xa1 && c < 0xff) {
    if (n < 2)
      return RET_TOOFEW(0);
    if (c == 0xa1 || c == 0xa2 || c == 0xad) {
      /* Code set 1 (JIS X 0208 Rows 1, 2 and NEC special characters) */
      unsigned char c1 = c;
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff) {
        unsigned char t1 = (c1 - 0xa1) >> 1;
        unsigned char t2 = (((c1 - 0xa1) & 1) ? 0x5e : 0) + (c2 - 0xa1);
        unsigned char buf[2];
        buf[0] = (t1 < 0x1f ? t1+0x81 : t1+0xc1);
        buf[1] = (t2 < 0x3f ? t2+0x40 : t2+0x41);
        return cp932ext_mbtowc(conv,pwc,buf,2);
      } else
        return RET_ILSEQ;
    }
    else if (c < 0xf5) {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff) {
        unsigned char buf[2];
        buf[0] = c-0x80; buf[1] = c2-0x80;
        return jisx0208_mbtowc(conv,pwc,buf,2);
      } else
        return RET_ILSEQ;
    } else {
      /* User-defined range. See
       * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff) {
        *pwc = 0xe000 + 94*(c-0xf5) + (c2-0xa1);
        return 2;
      } else
        return RET_ILSEQ;
    }
  }
  /* Code set 2 (half-width katakana) */
  if (c == 0x8e) {
    if (n < 2)
      return RET_TOOFEW(0);
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xe0) {
        int ret = jisx0201_mbtowc(conv,pwc,s+1,n-1);
        if (ret == RET_ILSEQ)
          return RET_ILSEQ;
        if (ret != 1) abort();
        return 2;
      } else
        return RET_ILSEQ;
    }
  }
  /* Code set 3 (JIS X 0212-1990) */
  if (c == 0x8f) {
    if (n < 2)
      return RET_TOOFEW(0);
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff) {
        if (n < 3)
          return RET_TOOFEW(0);
        if (c2 < 0xf3) {
          unsigned char c3 = s[2];
          if (c2 == 0xa2 && c3 == 0xc3) {
            /* eucJP-0212M.txt mapping. See
             * http://www.opengroup.or.jp/jvc/cde/eucJP-0212M.txt.970820 */
            *pwc = 0xffe4;
            return 3;
          } else if (c3 >= 0xa1 && c3 < 0xff) {
            unsigned char buf[2];
            int ret;
            buf[0] = c2-0x80; buf[1] = c3-0x80;
            ret = jisx0212_mbtowc(conv,pwc,buf,2);
            if (ret == RET_ILSEQ)
              return RET_ILSEQ;
            if (ret != 2) abort();
            return 3;
          } else
            return RET_ILSEQ;
        }
        else if (c2 >= 0xf3 && c2 < 0xf5) {
          /* IBM extentions */
          unsigned char c3 = s[2];
          if (c3 >= 0xa1 && c3 < 0xff) {
            unsigned char buf[2];
            int ret;
            buf[0] = c2; buf[1] = c3;
            ret = eucjp_msext_mbtowc(conv,pwc,buf,2);
            if (ret == RET_ILSEQ)
              return RET_ILSEQ;
            if (ret != 2) abort();
            return 3;
          } else
            return RET_ILSEQ;
        } else {
          /* User-defined range. See
           * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
          unsigned char c3 = s[2];
          if (c3 >= 0xa1 && c3 < 0xff) {
            *pwc = 0xe3ac + 94*(c2-0xf5) + (c3-0xa1);
            return 3;
          } else
            return RET_ILSEQ;
        }
      } else
        return RET_ILSEQ;
    }
  }
  return RET_ILSEQ;
}

static int
eucjp_ms_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  unsigned char buf[2];
  int ret;

  /* Code set 0 (ASCII or JIS X 0201-1976 Roman) */
  ret = ascii_wctomb(conv,r,wc,n);
  if (ret != RET_ILUNI)
    return ret;

  /* Code set 1 (JIS X 0208) */
  ret = jisx0208_wctomb(conv,buf,wc,2);
  if (ret != RET_ILUNI) {
    if (ret != 2) abort();
    if (n < 2)
      return RET_TOOSMALL;
    r[0] = buf[0]+0x80;
    r[1] = buf[1]+0x80;
    return 2;
  }

  /* Code set 1 (NEC special characters) */
  ret = cp932ext_wctomb(conv,buf,wc,2);
  if (ret != RET_ILUNI) {
    unsigned char s1, s2;
    if (ret != 2) abort();
    if (n < 2)
      return RET_TOOSMALL;
    s1 = buf[0];
    s2 = buf[1];
    if (s1 < 0xf0) {
      unsigned char t1 = (s1 < 0xe0 ? s1-0x81 : s1-0xc1);
      unsigned char t2 = (s2 < 0x80 ? s2-0x40 : s2-0x41);
      r[0] = 2*t1 + (t2 < 0x5e ? 0 : 1) + 0xa1;
      r[1] = (t2 < 0x5e ? t2 : t2-0x5e) + 0xa1;
      return 2;
    }
  }

  /* Code set 2 (half-width katakana) */
  ret = jisx0201_wctomb(conv,buf,wc,1);
  if (ret != RET_ILUNI && buf[0] >= 0x80) {
    if (ret != 1) abort();
    if (n < 2)
      return RET_TOOSMALL;
    r[0] = 0x8e;
    r[1] = buf[0];
    return 2;
  }

  /* Code set 3 (JIS X 0212-1990) */
  ret = jisx0212_wctomb(conv,buf,wc,2);
  if (ret != RET_ILUNI) {
    if (ret != 2) abort();
    if (n < 3)
      return RET_TOOSMALL;
    r[0] = 0x8f;
    r[1] = buf[0]+0x80;
    r[2] = buf[1]+0x80;
    return 3;
  }

  /* Code set 3 (IBM extensions) */
  ret = eucjp_msext_wctomb(conv,buf,wc,2);
  if (ret != RET_ILUNI) {
    if (ret != 2) abort();
    if (n < 3)
      return RET_TOOSMALL;
    r[0] = 0x8f;
    r[1] = buf[0];
    r[2] = buf[1];
    return 3;
  }

  /* Extra compatibility with Shift_JIS.  */
  if (wc == 0x00a5) {
    r[0] = 0x5c;
    return 1;
  }
  if (wc == 0x203e) {
    r[0] = 0x7e;
    return 1;
  }

  /* eucJP-0212M.txt mapping. See
   * http://www.opengroup.or.jp/jvc/cde/eucJP-0212M.txt.970820 */
  if (wc == 0xffe4) {
    r[0] = 0x8f;
    r[1] = 0xa2;
    r[2] = 0xc3;
    return 3;
  }

  /* User-defined range. See
   * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
  if (wc >= 0xe000 && wc < 0xe758) {
    if (wc < 0xe3ac) {
      unsigned char c1, c2;
      if (n < 2)
        return RET_TOOSMALL;
      c1 = (unsigned int) (wc - 0xe000) / 94;
      c2 = (unsigned int) (wc - 0xe000) % 94;
      r[0] = c1+0xf5;
      r[1] = c2+0xa1;
      return 2;
    } else {
      unsigned char c1, c2;
      if (n < 3)
        return RET_TOOSMALL;
      c1 = (unsigned int) (wc - 0xe3ac) / 94;
      c2 = (unsigned int) (wc - 0xe3ac) % 94;
      r[0] = 0x8f;
      r[1] = c1+0xf5;
      r[2] = c2+0xa1;
      return 3;
    }
  }

  return RET_ILUNI;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/lib/libiconv/dist/libiconv-1.12/lib/eucjp_ms.h $ $Rev: 680336 $")
#endif
