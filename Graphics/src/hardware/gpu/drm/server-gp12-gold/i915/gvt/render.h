/*
 * Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eddie Dong <eddie.dong@intel.com>
 *    Kevin Tian <kevin.tian@intel.com>
 *
 * Contributors:
 *    Zhi Wang <zhi.a.wang@intel.com>
 *    Changbin Du <changbin.du@intel.com>
 *    Zhenyu Wang <zhenyuw@linux.intel.com>
 *    Tina Zhang <tina.zhang@intel.com>
 *    Bing Niu <bing.niu@intel.com>
 *
 */

#ifndef __GVT_RENDER_H__
#define __GVT_RENDER_H__

void intel_gvt_load_render_mmio(struct intel_vgpu *vgpu, int ring_id);

void intel_gvt_restore_render_mmio(struct intel_vgpu *vgpu, int ring_id);

int intel_gvt_vgpu_conformance_check(struct intel_vgpu *vgpu, int ring_id);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/gvt/render.h $ $Rev: 845437 $")
#endif
