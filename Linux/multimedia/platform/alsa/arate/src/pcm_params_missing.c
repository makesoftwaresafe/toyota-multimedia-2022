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

static inline int hw_is_mask(snd_pcm_hw_param_t var)
{
	return var <= SND_PCM_HW_PARAM_LAST_MASK;
}

static inline int hw_is_interval(snd_pcm_hw_param_t var)
{
	return var >= SND_PCM_HW_PARAM_FIRST_INTERVAL &&
		var <= SND_PCM_HW_PARAM_LAST_INTERVAL;
}

#define hw_param_mask(params,var) \
	&((params)->masks[(var) - SND_PCM_HW_PARAM_FIRST_MASK])

#define hw_param_interval(params,var) \
	&((params)->intervals[(var) - SND_PCM_HW_PARAM_FIRST_INTERVAL])


#define hw_param_interval_c hw_param_interval


int _snd_pcm_hw_param_set_mask(snd_pcm_hw_params_t *params,
			       snd_pcm_hw_param_t var, const snd_mask_t *val)
{
	int changed;
	assert(hw_is_mask(var));
	changed = snd_mask_refine(hw_param_mask(params, var), val);
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}


int _snd_pcm_hw_param_set_minmax(snd_pcm_hw_params_t *params,
				 snd_pcm_hw_param_t var,
				 unsigned int min, int mindir,
				 unsigned int max, int maxdir)
{
	int changed, c1, c2;
	int openmin = 0, openmax = 0;
	if (mindir) {
		if (mindir > 0) {
			openmin = 1;
		} else if (mindir < 0) {
			if (min > 0) {
				openmin = 1;
				min--;
			}
		}
	}
	if (maxdir) {
		if (maxdir < 0) {
			openmax = 1;
		} else if (maxdir > 0) {
			openmax = 1;
			max++;
		}
	}
	if (hw_is_mask(var)) {
		snd_mask_t *mask = hw_param_mask(params, var);
		if (max == 0 && openmax) {
			snd_mask_none(mask);
			changed = -EINVAL;
		} else {
			c1 = snd_mask_refine_min(mask, min + !!openmin);
			if (c1 < 0)
				changed = c1;
			else {
				c2 = snd_mask_refine_max(mask, max - !!openmax);
				if (c2 < 0)
					changed = c2;
				else
					changed = (c1 || c2);
			}
		}
	}
	else if (hw_is_interval(var)) {
		snd_interval_t *i = hw_param_interval(params, var);
		c1 = snd_interval_refine_min(i, min, openmin);
		if (c1 < 0)
			changed = c1;
		else {
			c2 = snd_interval_refine_max(i, max, openmax);
			if (c2 < 0)
				changed = c2;
			else
				changed = (c1 || c2);
		}
	} else {
		assert(0);
		return -EINVAL;
	}
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}


/*deactivation review: CR-SWGIII-1336 */
/*PRQA: Lint Message 773 : Struct initializer must not be parentesized. Concatenation + stringify is safe as is*/
/*lint -save -e773 */
#define HW_PARAM(v) [SND_PCM_HW_PARAM_##v] = #v

static const char *const snd_pcm_hw_param_names[] = {
	HW_PARAM(ACCESS),
	HW_PARAM(FORMAT),
	HW_PARAM(SUBFORMAT),
	HW_PARAM(SAMPLE_BITS),
	HW_PARAM(FRAME_BITS),
	HW_PARAM(CHANNELS),
	HW_PARAM(RATE),
	HW_PARAM(PERIOD_TIME),
	HW_PARAM(PERIOD_SIZE),
	HW_PARAM(PERIOD_BYTES),
	HW_PARAM(PERIODS),
	HW_PARAM(BUFFER_TIME),
	HW_PARAM(BUFFER_SIZE),
	HW_PARAM(BUFFER_BYTES),
	HW_PARAM(TICK_TIME),
};
/*lint -restore */

const char *snd_pcm_hw_param_name(snd_pcm_hw_param_t param)
{
	assert(param <= SND_PCM_HW_PARAM_LAST_INTERVAL);
	return snd_pcm_hw_param_names[param];
}


typedef struct _snd_pcm_hw_rule snd_pcm_hw_rule_t;

typedef int (*snd_pcm_hw_rule_func_t)(snd_pcm_hw_params_t *params,
				      const snd_pcm_hw_rule_t *rule);

struct _snd_pcm_hw_rule {
	int var;
	snd_pcm_hw_rule_func_t func;
	int deps[4];
	void *private_data;
};

static int snd_pcm_hw_rule_mul(snd_pcm_hw_params_t *params,
			       const snd_pcm_hw_rule_t *rule)
{
	snd_interval_t t;
	snd_interval_mul(hw_param_interval_c(params, rule->deps[0]),
		     hw_param_interval_c(params, rule->deps[1]), &t);
	return snd_interval_refine(hw_param_interval(params, rule->var), &t);
}

static int snd_pcm_hw_rule_div(snd_pcm_hw_params_t *params,
			const snd_pcm_hw_rule_t *rule)
{
	snd_interval_t t;
	snd_interval_div(hw_param_interval_c(params, rule->deps[0]),
		     hw_param_interval_c(params, rule->deps[1]), &t);
	return snd_interval_refine(hw_param_interval(params, rule->var), &t);
}

static int snd_pcm_hw_rule_muldivk(snd_pcm_hw_params_t *params,
				   const snd_pcm_hw_rule_t *rule)
{
	snd_interval_t t;
	snd_interval_muldivk(hw_param_interval_c(params, rule->deps[0]),
			 hw_param_interval_c(params, rule->deps[1]),
			 (unsigned long) rule->private_data, &t);
	return snd_interval_refine(hw_param_interval(params, rule->var), &t);
}

static int snd_pcm_hw_rule_mulkdiv(snd_pcm_hw_params_t *params,
				   const snd_pcm_hw_rule_t *rule)
{
	snd_interval_t t;
	snd_interval_mulkdiv(hw_param_interval_c(params, rule->deps[0]),
			 (unsigned long) rule->private_data,
			 hw_param_interval_c(params, rule->deps[1]), &t);
	return snd_interval_refine(hw_param_interval(params, rule->var), &t);
}

static int snd_pcm_hw_rule_format(snd_pcm_hw_params_t *params,
				  const snd_pcm_hw_rule_t *rule)
{
	int changed = 0;
	snd_pcm_format_t k;
	snd_mask_t *mask = hw_param_mask(params, rule->var);
	snd_interval_t *i = hw_param_interval(params, rule->deps[0]);
	for (k = (snd_pcm_format_t)0; k <= SND_PCM_FORMAT_LAST; k++) {
		int bits;
		if (!snd_pcm_format_mask_test(mask, k))
			continue;
		bits = snd_pcm_format_physical_width(k);
		if (bits < 0)
			continue;
		if (!snd_interval_test(i, (unsigned int) bits)) {
			snd_pcm_format_mask_reset(mask, k);
			if (snd_mask_empty(mask))
				return -EINVAL;
			changed = 1;
		}
	}
	return changed;
}


static int snd_pcm_hw_rule_sample_bits(snd_pcm_hw_params_t *params,
				       const snd_pcm_hw_rule_t *rule)
{
	unsigned int min, max;
	snd_pcm_format_t k;
	snd_interval_t *i = hw_param_interval(params, rule->var);
	snd_mask_t *mask = hw_param_mask(params, rule->deps[0]);
	int c, changed = 0;
	min = UINT_MAX;
	max = 0;
	for (k = (snd_pcm_format_t)0; k <= SND_PCM_FORMAT_LAST; k++) {
		int bits;
		if (!snd_pcm_format_mask_test(mask, k))
			continue;
		bits = snd_pcm_format_physical_width(k);
		if (bits < 0)
			continue;
		if (min > (unsigned)bits)
			min = bits;
		if (max < (unsigned)bits)
			max = bits;
	}
	c = snd_interval_refine_min(i, min, 0);
	if (c < 0)
		return c;
	if (c)
		changed = 1;
	c = snd_interval_refine_max(i, max, 0);
	if (c < 0)
		return c;
	if (c)
		changed = 1;
	return changed;
}

static const snd_pcm_hw_rule_t refine_rules[] = {
	{
		.var = SND_PCM_HW_PARAM_FORMAT,
		.func = snd_pcm_hw_rule_format,
		.deps = { SND_PCM_HW_PARAM_SAMPLE_BITS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_SAMPLE_BITS, 
		.func = snd_pcm_hw_rule_sample_bits,
		.deps = { SND_PCM_HW_PARAM_FORMAT, 
			SND_PCM_HW_PARAM_SAMPLE_BITS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_SAMPLE_BITS, 
		.func = snd_pcm_hw_rule_div,
		.deps = { SND_PCM_HW_PARAM_FRAME_BITS,
			SND_PCM_HW_PARAM_CHANNELS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_FRAME_BITS, 
		.func = snd_pcm_hw_rule_mul,
		.deps = { SND_PCM_HW_PARAM_SAMPLE_BITS,
			SND_PCM_HW_PARAM_CHANNELS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_FRAME_BITS, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_PERIOD_BYTES,
			SND_PCM_HW_PARAM_PERIOD_SIZE, -1 },
		.private_data = (void*) 8,
	},
	{
		.var = SND_PCM_HW_PARAM_FRAME_BITS, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_BUFFER_BYTES,
			SND_PCM_HW_PARAM_BUFFER_SIZE, -1 },
		.private_data = (void*) 8,
	},
	{
		.var = SND_PCM_HW_PARAM_CHANNELS, 
		.func = snd_pcm_hw_rule_div,
		.deps = { SND_PCM_HW_PARAM_FRAME_BITS,
			SND_PCM_HW_PARAM_SAMPLE_BITS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_RATE, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_PERIOD_SIZE,
			SND_PCM_HW_PARAM_PERIOD_TIME, -1 },
		.private_data = (void*) 1000000,
	},
	{
		.var = SND_PCM_HW_PARAM_RATE, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_BUFFER_SIZE,
			SND_PCM_HW_PARAM_BUFFER_TIME, -1 },
		.private_data = (void*) 1000000,
	},
	{
		.var = SND_PCM_HW_PARAM_PERIODS, 
		.func = snd_pcm_hw_rule_div,
		.deps = { SND_PCM_HW_PARAM_BUFFER_SIZE,
			SND_PCM_HW_PARAM_PERIOD_SIZE, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_PERIOD_SIZE, 
		.func = snd_pcm_hw_rule_div,
		.deps = { SND_PCM_HW_PARAM_BUFFER_SIZE,
			SND_PCM_HW_PARAM_PERIODS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_PERIOD_SIZE, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_PERIOD_BYTES,
			SND_PCM_HW_PARAM_FRAME_BITS, -1 },
		.private_data = (void*) 8,
	},
	{
		.var = SND_PCM_HW_PARAM_PERIOD_SIZE, 
		.func = snd_pcm_hw_rule_muldivk,
		.deps = { SND_PCM_HW_PARAM_PERIOD_TIME,
			SND_PCM_HW_PARAM_RATE, -1 },
		.private_data = (void*) 1000000,
	},
	{
		.var = SND_PCM_HW_PARAM_BUFFER_SIZE, 
		.func = snd_pcm_hw_rule_mul,
		.deps = { SND_PCM_HW_PARAM_PERIOD_SIZE,
			SND_PCM_HW_PARAM_PERIODS, -1 },
		.private_data = 0,
	},
	{
		.var = SND_PCM_HW_PARAM_BUFFER_SIZE, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_BUFFER_BYTES,
			SND_PCM_HW_PARAM_FRAME_BITS, -1 },
		.private_data = (void*) 8,
	},
	{
		.var = SND_PCM_HW_PARAM_BUFFER_SIZE, 
		.func = snd_pcm_hw_rule_muldivk,
		.deps = { SND_PCM_HW_PARAM_BUFFER_TIME,
			SND_PCM_HW_PARAM_RATE, -1 },
		.private_data = (void*) 1000000,
	},
	{
		.var = SND_PCM_HW_PARAM_PERIOD_BYTES, 
		.func = snd_pcm_hw_rule_muldivk,
		.deps = { SND_PCM_HW_PARAM_PERIOD_SIZE,
			SND_PCM_HW_PARAM_FRAME_BITS, -1 },
		.private_data = (void*) 8,
	},
	{
		.var = SND_PCM_HW_PARAM_BUFFER_BYTES, 
		.func = snd_pcm_hw_rule_muldivk,
		.deps = { SND_PCM_HW_PARAM_BUFFER_SIZE,
			SND_PCM_HW_PARAM_FRAME_BITS, -1 },
		.private_data = (void*) 8,
	},
	{
		.var = SND_PCM_HW_PARAM_PERIOD_TIME, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_PERIOD_SIZE,
			SND_PCM_HW_PARAM_RATE, -1 },
		.private_data = (void*) 1000000,
	},
	{
		.var = SND_PCM_HW_PARAM_BUFFER_TIME, 
		.func = snd_pcm_hw_rule_mulkdiv,
		.deps = { SND_PCM_HW_PARAM_BUFFER_SIZE,
			SND_PCM_HW_PARAM_RATE, -1 },
		.private_data = (void*) 1000000,
	},
};

#define RULES (sizeof(refine_rules) / sizeof(refine_rules[0]))

static const snd_mask_t refine_masks[(SND_PCM_HW_PARAM_LAST_MASK - SND_PCM_HW_PARAM_FIRST_MASK) + 1] = {
	[SND_PCM_HW_PARAM_ACCESS - SND_PCM_HW_PARAM_FIRST_MASK] = {
		.bits = { 0x1f },
	},
	[SND_PCM_HW_PARAM_FORMAT - SND_PCM_HW_PARAM_FIRST_MASK] = {
		.bits = { 0x81ffffff, 0xfff},
	},
	[SND_PCM_HW_PARAM_SUBFORMAT - SND_PCM_HW_PARAM_FIRST_MASK] = {
		.bits = { 0x1 },
	},
};
  
static const snd_interval_t refine_intervals[(SND_PCM_HW_PARAM_LAST_INTERVAL - SND_PCM_HW_PARAM_FIRST_INTERVAL) + 1] = {
	[SND_PCM_HW_PARAM_SAMPLE_BITS - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 1, .empty = 0,
	},
	[SND_PCM_HW_PARAM_FRAME_BITS - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 1, .empty = 0,
	},
	[SND_PCM_HW_PARAM_CHANNELS - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 1, .empty = 0,
	},
	[SND_PCM_HW_PARAM_RATE - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
	[SND_PCM_HW_PARAM_PERIOD_TIME - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 0, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
	[SND_PCM_HW_PARAM_PERIOD_SIZE - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 0, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
	[SND_PCM_HW_PARAM_PERIOD_BYTES - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 0, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
	[SND_PCM_HW_PARAM_PERIODS - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 0, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
	[SND_PCM_HW_PARAM_BUFFER_TIME - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
	[SND_PCM_HW_PARAM_BUFFER_SIZE - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 1, .empty = 0,
	},
	[SND_PCM_HW_PARAM_BUFFER_BYTES - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 1, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 1, .empty = 0,
	},
	[SND_PCM_HW_PARAM_TICK_TIME - SND_PCM_HW_PARAM_FIRST_INTERVAL] = {
		.min = 0, .max = UINT_MAX,
		.openmin = 0, .openmax = 0, .integer = 0, .empty = 0,
	},
};

#if 0
#define RULES_DEBUG
#endif

int snd_pcm_hw_refine_soft(snd_pcm_t *pcm ATTRIBUTE_UNUSED, snd_pcm_hw_params_t *params)
{
	unsigned int k;
	snd_interval_t *i;
	unsigned int rstamps[RULES];
	unsigned int vstamps[SND_PCM_HW_PARAM_LAST_INTERVAL + 1];
	unsigned int stamp = 2;
	int changed, again;
#ifdef RULES_DEBUG
	snd_output_t *log;
	snd_output_stdio_attach(&log, stderr, 0);
	snd_output_printf(log, "refine_soft '%s' (begin)\n", pcm->name);
	snd_pcm_hw_params_dump(params, log);
#else
	pcm = pcm;
#endif

	for (k = SND_PCM_HW_PARAM_FIRST_MASK; k <= SND_PCM_HW_PARAM_LAST_MASK; k++) {
		if (!(params->rmask & (1 << k)))
			continue;
		changed = snd_mask_refine(hw_param_mask(params, k),
					  &refine_masks[k - SND_PCM_HW_PARAM_FIRST_MASK]);
		if (changed)
			params->cmask |= 1 << k;
		if (changed < 0)
			goto _err;
	}

	for (k = SND_PCM_HW_PARAM_FIRST_INTERVAL; k <= SND_PCM_HW_PARAM_LAST_INTERVAL; k++) {
		if (!(params->rmask & (1 << k)))
			continue;
		changed = snd_interval_refine(hw_param_interval(params, k),
				      &refine_intervals[k - SND_PCM_HW_PARAM_FIRST_INTERVAL]);
		if (changed)
			params->cmask |= 1 << k;
		if (changed < 0)
			goto _err;
	}

	for (k = 0; k < RULES; k++)
		rstamps[k] = 0;
	for (k = 0; k <= SND_PCM_HW_PARAM_LAST_INTERVAL; k++)
		vstamps[k] = (params->rmask & (1 << k)) ? 1 : 0;
	do {
		again = 0;
		for (k = 0; k < RULES; k++) {
			const snd_pcm_hw_rule_t *r = &refine_rules[k];
			unsigned int d;
			int doit = 0;
			for (d = 0; r->deps[d] >= 0; d++) {
				if (vstamps[r->deps[d]] > rstamps[k]) {
					doit = 1;
					break;
				}
			}
			if (!doit)
				continue;
#ifdef RULES_DEBUG
			snd_output_printf(log, "Rule %d (%p): ", k, r->func);
			if (r->var >= 0) {
				snd_output_printf(log, "%s=", snd_pcm_hw_param_name(r->var));
				snd_pcm_hw_param_dump(params, r->var, log);
				snd_output_puts(log, " -> ");
			}
#endif
			changed = r->func(params, r);
#ifdef RULES_DEBUG
			if (r->var >= 0)
				snd_pcm_hw_param_dump(params, r->var, log);
			for (d = 0; r->deps[d] >= 0; d++) {
				snd_output_printf(log, " %s=", snd_pcm_hw_param_name(r->deps[d]));
				snd_pcm_hw_param_dump(params, r->deps[d], log);
			}
			snd_output_putc(log, '\n');
#endif
			rstamps[k] = stamp;
			if (changed && r->var >= 0) {
				params->cmask |= 1 << r->var;
				vstamps[r->var] = stamp;
				again = 1;
			}
			if (changed < 0)
				goto _err;
			stamp++;
		}
	} while (again);
	if (!params->msbits) {
		i = hw_param_interval(params, SND_PCM_HW_PARAM_SAMPLE_BITS);
		if (snd_interval_single(i))
			params->msbits = snd_interval_value(i);
	}

	if (!params->rate_den) {
		i = hw_param_interval(params, SND_PCM_HW_PARAM_RATE);
		if (snd_interval_single(i)) {
			params->rate_num = snd_interval_value(i);
			params->rate_den = 1;
		}
	}
	params->rmask = 0;
	return 0;
 _err:
#ifdef RULES_DEBUG
	snd_output_printf(log, "refine_soft '%s' (end-%i)\n", pcm->name, changed);
	snd_pcm_hw_params_dump(params, log);
	snd_output_close(log);
#endif
	return changed;
}

