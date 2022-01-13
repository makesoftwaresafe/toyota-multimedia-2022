/*
 *  Internal functions
 *  Copyright (c) 2000 by Abramo Bagnara <abramo@alsa-project.org>
 * 			Jaroslav Kysela <perex@perex.cz>
 *
 *   This file contains copies of alsalib internal functions needed to build plugin arate.
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <time.h>
#include <sys/types.h>
#include <limits.h>
#include "pcm_local.h"


static inline void div64_32(u_int64_t *n, u_int32_t d, u_int32_t *rem)
{
	*rem = *n % d;
	*n /= d;
}

static inline unsigned int div32(unsigned int a, unsigned int b, 
				 unsigned int *r)
{
	if (b == 0) {
		*r = 0;
		return UINT_MAX;
	}
	*r = a % b;
	return a / b;
}

static inline unsigned int div_down(unsigned int a, unsigned int b)
{
	if (b == 0)
		return UINT_MAX;
	return a / b;
}

static inline unsigned int mul(unsigned int a, unsigned int b)
{
	if (a == 0)
		return 0;
	if (div_down(UINT_MAX, a) < b)
		return UINT_MAX;
	return a * b;
}

static inline unsigned int muldiv32(unsigned int a, unsigned int b,
				    unsigned int c, unsigned int *r)
{
	u_int64_t n = (u_int64_t) a * b;
	if (c == 0) {
		assert(n > 0);
		*r = 0;
		return UINT_MAX;
	}
	div64_32(&n, c, r);
	if (n >= UINT_MAX) {
		*r = 0;
		return UINT_MAX;
	}
	return n;
}

int snd_interval_refine_min(snd_interval_t *i, unsigned int min, int openmin)
{
	int changed = 0;
	if (snd_interval_empty(i))
		return -ENOENT;
	if (i->min < min) {
		i->min = min;
		i->openmin = openmin;
		changed = 1;
	} else if (i->min == min && !i->openmin && openmin) {
		i->openmin = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmin) {
			i->min++;
			i->openmin = 0;
		}
	}
	if (snd_interval_checkempty(i)) {
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

int snd_interval_refine_max(snd_interval_t *i, unsigned int max, int openmax)
{
	int changed = 0;
	if (snd_interval_empty(i))
		return -ENOENT;
	if (i->max > max) {
		i->max = max;
		i->openmax = openmax;
		changed = 1;
	} else if (i->max == max && !i->openmax && openmax) {
		i->openmax = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmax) {
			i->max--;
			i->openmax = 0;
		}
	}
	if (snd_interval_checkempty(i)) {
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}


/* r <- v */
int snd_interval_refine(snd_interval_t *i, const snd_interval_t *v)
{
	int changed = 0;
	if (snd_interval_empty(i))
		return -ENOENT;
	if (i->min < v->min) {
		i->min = v->min;
		i->openmin = v->openmin;
		changed = 1;
	} else if (i->min == v->min && !i->openmin && v->openmin) {
		i->openmin = 1;
		changed = 1;
	}
	if (i->max > v->max) {
		i->max = v->max;
		i->openmax = v->openmax;
		changed = 1;
	} else if (i->max == v->max && !i->openmax && v->openmax) {
		i->openmax = 1;
		changed = 1;
	}
	if (!i->integer && v->integer) {
		i->integer = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmin) {
			i->min++;
			i->openmin = 0;
		}
		if (i->openmax) {
			i->max--;
			i->openmax = 0;
		}
	} else if (!i->openmin && !i->openmax && i->min == i->max)
		i->integer = 1;
	if (snd_interval_checkempty(i)) {
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

void snd_interval_mul(const snd_interval_t *a, const snd_interval_t *b, snd_interval_t *c)
{
	if (a->empty || b->empty) {
		snd_interval_none(c);
		return;
	}
	c->empty = 0;
	c->min = mul(a->min, b->min);
	c->openmin = (a->openmin || b->openmin);
	c->max = mul(a->max,  b->max);
	c->openmax = (a->openmax || b->openmax);
	c->integer = (a->integer && b->integer);
}

void snd_interval_div(const snd_interval_t *a, const snd_interval_t *b, snd_interval_t *c)
{
	unsigned int r;
	if (a->empty || b->empty) {
		snd_interval_none(c);
		return;
	}
	c->empty = 0;
	c->min = div32(a->min, b->max, &r);
	c->openmin = (r || a->openmin || b->openmax);
	if (b->min > 0) {
		c->max = div32(a->max, b->min, &r);
		if (r) {
			c->max++;
			c->openmax = 1;
		} else
			c->openmax = (a->openmax || b->openmin);
	} else {
		c->max = UINT_MAX;
		c->openmax = 0;
	}
	c->integer = 0;
}

/* a * b / k */
void snd_interval_muldivk(const snd_interval_t *a, const snd_interval_t *b,
		      unsigned int k, snd_interval_t *c)
{
	unsigned int r;
	if (a->empty || b->empty) {
		snd_interval_none(c);
		return;
	}
	c->empty = 0;
	c->min = muldiv32(a->min, b->min, k, &r);
	c->openmin = (r || a->openmin || b->openmin);
	c->max = muldiv32(a->max, b->max, k, &r);
	if (r) {
		c->max++;
		c->openmax = 1;
	} else
		c->openmax = (a->openmax || b->openmax);
	c->integer = 0;
}

/* a * k / b */
void snd_interval_mulkdiv(const snd_interval_t *a, unsigned int k,
		      const snd_interval_t *b, snd_interval_t *c)
{
	unsigned int r;
	if (a->empty || b->empty) {
		snd_interval_none(c);
		return;
	}
	c->empty = 0;
	c->min = muldiv32(a->min, k, b->max, &r);
	c->openmin = (r || a->openmin || b->openmax);
	if (b->min > 0) {
		c->max = muldiv32(a->max, k, b->min, &r);
		if (r) {
			c->max++;
			c->openmax = 1;
		} else
			c->openmax = (a->openmax || b->openmin);
	} else {
		c->max = UINT_MAX;
		c->openmax = 0;
	}
	c->integer = 0;
}

