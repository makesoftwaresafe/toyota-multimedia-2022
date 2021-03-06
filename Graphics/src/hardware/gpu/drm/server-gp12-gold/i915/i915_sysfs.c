/*
 * Copyright © 2012 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Ben Widawsky <ben@bwidawsk.net>
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include "intel_drv.h"
#include "i915_drv.h"

static inline struct drm_i915_private *kdev_minor_to_i915(struct device *kdev)
{
	struct drm_minor *minor = dev_get_drvdata(kdev);
	return to_i915(minor->dev);
}

#ifdef CONFIG_PM
static u32 calc_residency(struct drm_i915_private *dev_priv,
			  i915_reg_t reg)
{
	u64 raw_time; /* 32b value may overflow during fixed point math */
	u64 units = 128ULL, div = 100000ULL;
	u32 ret;

	if (!intel_enable_rc6())
		return 0;

	intel_runtime_pm_get(dev_priv);

	/* On VLV and CHV, residency time is in CZ units rather than 1.28us */
	if (IS_VALLEYVIEW(dev_priv) || IS_CHERRYVIEW(dev_priv)) {
		units = 1;
		div = dev_priv->czclk_freq;

		if (I915_READ(VLV_COUNTER_CONTROL) & VLV_COUNT_RANGE_HIGH)
			units <<= 8;
	} else if (IS_GEN9_LP(dev_priv)) {
		units = 1;
		div = 1200;		/* 833.33ns */
	}

	raw_time = I915_READ(reg) * units;
	ret = DIV_ROUND_UP_ULL(raw_time, div);

	intel_runtime_pm_put(dev_priv);
	return ret;
}

static ssize_t
show_rc6_mask(struct device *kdev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", intel_enable_rc6());
}

static ssize_t
show_rc6_ms(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 rc6_residency = calc_residency(dev_priv, GEN6_GT_GFX_RC6);
	return snprintf(buf, PAGE_SIZE, "%u\n", rc6_residency);
}

static ssize_t
show_rc6p_ms(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 rc6p_residency = calc_residency(dev_priv, GEN6_GT_GFX_RC6p);
	return snprintf(buf, PAGE_SIZE, "%u\n", rc6p_residency);
}

static ssize_t
show_rc6pp_ms(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 rc6pp_residency = calc_residency(dev_priv, GEN6_GT_GFX_RC6pp);
	return snprintf(buf, PAGE_SIZE, "%u\n", rc6pp_residency);
}

static ssize_t
show_media_rc6_ms(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 rc6_residency = calc_residency(dev_priv, VLV_GT_MEDIA_RC6);
	return snprintf(buf, PAGE_SIZE, "%u\n", rc6_residency);
}

static DEVICE_ATTR(rc6_enable, S_IRUGO, show_rc6_mask, NULL);
static DEVICE_ATTR(rc6_residency_ms, S_IRUGO, show_rc6_ms, NULL);
static DEVICE_ATTR(rc6p_residency_ms, S_IRUGO, show_rc6p_ms, NULL);
static DEVICE_ATTR(rc6pp_residency_ms, S_IRUGO, show_rc6pp_ms, NULL);
static DEVICE_ATTR(media_rc6_residency_ms, S_IRUGO, show_media_rc6_ms, NULL);

static struct attribute *rc6_attrs[] = {
	&dev_attr_rc6_enable.attr,
	&dev_attr_rc6_residency_ms.attr,
	NULL
};

static struct attribute_group rc6_attr_group = {
	.name = power_group_name,
	.attrs =  rc6_attrs
};

static struct attribute *rc6p_attrs[] = {
	&dev_attr_rc6p_residency_ms.attr,
	&dev_attr_rc6pp_residency_ms.attr,
	NULL
};

static struct attribute_group rc6p_attr_group = {
	.name = power_group_name,
	.attrs =  rc6p_attrs
};

static struct attribute *media_rc6_attrs[] = {
	&dev_attr_media_rc6_residency_ms.attr,
	NULL
};

static struct attribute_group media_rc6_attr_group = {
	.name = power_group_name,
	.attrs =  media_rc6_attrs
};
#endif

static int l3_access_valid(struct drm_i915_private *dev_priv, loff_t offset)
{
	if (!HAS_L3_DPF(dev_priv))
		return -EPERM;

	if (offset % 4 != 0)
		return -EINVAL;

	if (offset >= GEN7_L3LOG_SIZE)
		return -ENXIO;

	return 0;
}

static ssize_t
i915_l3_read(struct file *filp, struct kobject *kobj,
	     struct bin_attribute *attr, char *buf,
	     loff_t offset, size_t count)
{
	struct device *kdev = kobj_to_dev(kobj);
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	struct drm_device *dev = &dev_priv->drm;
	int slice = (int)(uintptr_t)attr->private;
	int ret;

	count = round_down(count, 4);

	ret = l3_access_valid(dev_priv, offset);
	if (ret)
		return ret;

	count = min_t(size_t, GEN7_L3LOG_SIZE - offset, count);

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	if (dev_priv->l3_parity.remap_info[slice])
		memcpy(buf,
		       dev_priv->l3_parity.remap_info[slice] + (offset/4),
		       count);
	else
		memset(buf, 0, count);

	mutex_unlock(&dev->struct_mutex);

	return count;
}

static ssize_t
i915_l3_write(struct file *filp, struct kobject *kobj,
	      struct bin_attribute *attr, char *buf,
	      loff_t offset, size_t count)
{
	struct device *kdev = kobj_to_dev(kobj);
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	struct drm_device *dev = &dev_priv->drm;
	struct i915_gem_context *ctx;
	u32 *temp = NULL; /* Just here to make handling failures easy */
	int slice = (int)(uintptr_t)attr->private;
	int ret;

	if (!HAS_HW_CONTEXTS(dev_priv))
		return -ENXIO;

	ret = l3_access_valid(dev_priv, offset);
	if (ret)
		return ret;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	if (!dev_priv->l3_parity.remap_info[slice]) {
		temp = kzalloc(GEN7_L3LOG_SIZE, GFP_KERNEL);
		if (!temp) {
			mutex_unlock(&dev->struct_mutex);
			return -ENOMEM;
		}
	}

	/* TODO: Ideally we really want a GPU reset here to make sure errors
	 * aren't propagated. Since I cannot find a stable way to reset the GPU
	 * at this point it is left as a TODO.
	*/
	if (temp)
		dev_priv->l3_parity.remap_info[slice] = temp;

	memcpy(dev_priv->l3_parity.remap_info[slice] + (offset/4), buf, count);

	/* NB: We defer the remapping until we switch to the context */
	list_for_each_entry(ctx, &dev_priv->context_list, link)
		ctx->remap_slice |= (1<<slice);

	mutex_unlock(&dev->struct_mutex);

	return count;
}

static struct bin_attribute dpf_attrs = {
	.attr = {.name = "l3_parity", .mode = (S_IRUSR | S_IWUSR)},
	.size = GEN7_L3LOG_SIZE,
	.read = i915_l3_read,
	.write = i915_l3_write,
	.mmap = NULL,
	.private = (void *)0
};

static struct bin_attribute dpf_attrs_1 = {
	.attr = {.name = "l3_parity_slice_1", .mode = (S_IRUSR | S_IWUSR)},
	.size = GEN7_L3LOG_SIZE,
	.read = i915_l3_read,
	.write = i915_l3_write,
	.mmap = NULL,
	.private = (void *)1
};

static ssize_t gt_act_freq_mhz_show(struct device *kdev,
				    struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	int ret;

	intel_runtime_pm_get(dev_priv);

	mutex_lock(&dev_priv->rps.hw_lock);
	if (IS_VALLEYVIEW(dev_priv) || IS_CHERRYVIEW(dev_priv)) {
		u32 freq;
		freq = vlv_punit_read(dev_priv, PUNIT_REG_GPU_FREQ_STS);
		ret = intel_gpu_freq(dev_priv, (freq >> 8) & 0xff);
	} else {
		u32 rpstat = I915_READ(GEN6_RPSTAT1);
		if (IS_GEN9(dev_priv))
			ret = (rpstat & GEN9_CAGF_MASK) >> GEN9_CAGF_SHIFT;
		else if (IS_HASWELL(dev_priv) || IS_BROADWELL(dev_priv))
			ret = (rpstat & HSW_CAGF_MASK) >> HSW_CAGF_SHIFT;
		else
			ret = (rpstat & GEN6_CAGF_MASK) >> GEN6_CAGF_SHIFT;
		ret = intel_gpu_freq(dev_priv, ret);
	}
	mutex_unlock(&dev_priv->rps.hw_lock);

	intel_runtime_pm_put(dev_priv);

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t gt_cur_freq_mhz_show(struct device *kdev,
				    struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			intel_gpu_freq(dev_priv,
				       dev_priv->rps.cur_freq));
}

static ssize_t gt_boost_freq_mhz_show(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			intel_gpu_freq(dev_priv,
				       dev_priv->rps.boost_freq));
}

static ssize_t gt_boost_freq_mhz_store(struct device *kdev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 val;
	ssize_t ret;

	ret = kstrtou32(buf, 0, &val);
	if (ret)
		return ret;

	/* Validate against (static) hardware limits */
	val = intel_freq_opcode(dev_priv, val);
	if (val < dev_priv->rps.min_freq || val > dev_priv->rps.max_freq)
		return -EINVAL;

	mutex_lock(&dev_priv->rps.hw_lock);
	dev_priv->rps.boost_freq = val;
	mutex_unlock(&dev_priv->rps.hw_lock);

	return count;
}

static ssize_t vlv_rpe_freq_mhz_show(struct device *kdev,
				     struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			intel_gpu_freq(dev_priv,
				       dev_priv->rps.efficient_freq));
}

static ssize_t gt_max_freq_mhz_show(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			intel_gpu_freq(dev_priv,
				       dev_priv->rps.max_freq_softlimit));
}

static ssize_t gt_max_freq_mhz_store(struct device *kdev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 val;
	ssize_t ret;

	ret = kstrtou32(buf, 0, &val);
	if (ret)
		return ret;

	intel_runtime_pm_get(dev_priv);

	mutex_lock(&dev_priv->rps.hw_lock);

	val = intel_freq_opcode(dev_priv, val);

	if (val < dev_priv->rps.min_freq ||
	    val > dev_priv->rps.max_freq ||
	    val < dev_priv->rps.min_freq_softlimit) {
		mutex_unlock(&dev_priv->rps.hw_lock);
		intel_runtime_pm_put(dev_priv);
		return -EINVAL;
	}

	if (val > dev_priv->rps.rp0_freq)
		DRM_DEBUG("User requested overclocking to %d\n",
			  intel_gpu_freq(dev_priv, val));

	dev_priv->rps.max_freq_softlimit = val;

	val = clamp_t(int, dev_priv->rps.cur_freq,
		      dev_priv->rps.min_freq_softlimit,
		      dev_priv->rps.max_freq_softlimit);

	/* We still need *_set_rps to process the new max_delay and
	 * update the interrupt limits and PMINTRMSK even though
	 * frequency request may be unchanged. */
	intel_set_rps(dev_priv, val);

	mutex_unlock(&dev_priv->rps.hw_lock);

	intel_runtime_pm_put(dev_priv);

	return count;
}

static ssize_t gt_min_freq_mhz_show(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			intel_gpu_freq(dev_priv,
				       dev_priv->rps.min_freq_softlimit));
}

static ssize_t gt_min_freq_mhz_store(struct device *kdev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 val;
	ssize_t ret;

	ret = kstrtou32(buf, 0, &val);
	if (ret)
		return ret;

	intel_runtime_pm_get(dev_priv);

	mutex_lock(&dev_priv->rps.hw_lock);

	val = intel_freq_opcode(dev_priv, val);

	if (val < dev_priv->rps.min_freq ||
	    val > dev_priv->rps.max_freq ||
	    val > dev_priv->rps.max_freq_softlimit) {
		mutex_unlock(&dev_priv->rps.hw_lock);
		intel_runtime_pm_put(dev_priv);
		return -EINVAL;
	}

	dev_priv->rps.min_freq_softlimit = val;

	val = clamp_t(int, dev_priv->rps.cur_freq,
		      dev_priv->rps.min_freq_softlimit,
		      dev_priv->rps.max_freq_softlimit);

	/* We still need *_set_rps to process the new min_delay and
	 * update the interrupt limits and PMINTRMSK even though
	 * frequency request may be unchanged. */
	intel_set_rps(dev_priv, val);

	mutex_unlock(&dev_priv->rps.hw_lock);

	intel_runtime_pm_put(dev_priv);

	return count;

}

#define YES_NO(cond) (cond) ? "Yes" : "No"

static ssize_t kick_delayed_init_show(struct device *kdev,
				      struct device_attribute *attr, char *buf)
{
	/*
	struct drm_minor *minor = dev_to_drm_minor(kdev);
	struct drm_device *dev = minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	*/

	return snprintf(buf, PAGE_SIZE,
			"delay initialization by:    %d ms\n"
			"driver registration         %s\n"
			"GMBUS MISC pin registration %s\n"
			"DDI Port A initialization   %s\n"
			"DDI Port B initialization   %s\n"
			"DDI Port C initialization   %s\n"
			"MIPI DSI initialization     %s\n"
			"Abort on first use          %s\n",
			i915.tsd_delay,
			YES_NO(i915.tsd_init & TSD_INIT_DELAY_REGISTER),
			YES_NO(i915.tsd_init & TSD_INIT_SKIP_PIN_MISC),
			YES_NO(i915.tsd_init & TSD_INIT_DELAY_DDI_A),
			YES_NO(i915.tsd_init & TSD_INIT_DELAY_DDI_B),
			YES_NO(i915.tsd_init & TSD_INIT_DELAY_DDI_C),
			YES_NO(i915.tsd_init & TSD_INIT_DELAY_DSI),
			YES_NO(i915.tsd_init & TSD_INIT_ABORT_ON_USE)
			);
}

static ssize_t kick_delayed_init_store(struct device *kdev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct drm_minor *minor = dev_get_drvdata(kdev);
        struct drm_i915_private *dev_priv = to_i915(minor->dev);

	flush_delayed_work(&dev_priv->tsd_init);

	return count;
}


static DEVICE_ATTR(gt_act_freq_mhz, S_IRUGO, gt_act_freq_mhz_show, NULL);
static DEVICE_ATTR(gt_cur_freq_mhz, S_IRUGO, gt_cur_freq_mhz_show, NULL);
static DEVICE_ATTR(gt_boost_freq_mhz, S_IRUGO | S_IWUSR, gt_boost_freq_mhz_show, gt_boost_freq_mhz_store);
static DEVICE_ATTR(gt_max_freq_mhz, S_IRUGO | S_IWUSR, gt_max_freq_mhz_show, gt_max_freq_mhz_store);
static DEVICE_ATTR(gt_min_freq_mhz, S_IRUGO | S_IWUSR, gt_min_freq_mhz_show, gt_min_freq_mhz_store);

static DEVICE_ATTR(vlv_rpe_freq_mhz, S_IRUGO, vlv_rpe_freq_mhz_show, NULL);

static ssize_t gt_rp_mhz_show(struct device *kdev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(gt_RP0_freq_mhz, S_IRUGO, gt_rp_mhz_show, NULL);
static DEVICE_ATTR(gt_RP1_freq_mhz, S_IRUGO, gt_rp_mhz_show, NULL);
static DEVICE_ATTR(gt_RPn_freq_mhz, S_IRUGO, gt_rp_mhz_show, NULL);
static DEVICE_ATTR(kick_delayed_init, S_IRUGO | S_IWUSR,
		   kick_delayed_init_show, kick_delayed_init_store);

/* For now we have a static number of RP states */
static ssize_t gt_rp_mhz_show(struct device *kdev, struct device_attribute *attr, char *buf)
{
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	u32 val;

	if (attr == &dev_attr_gt_RP0_freq_mhz)
		val = intel_gpu_freq(dev_priv, dev_priv->rps.rp0_freq);
	else if (attr == &dev_attr_gt_RP1_freq_mhz)
		val = intel_gpu_freq(dev_priv, dev_priv->rps.rp1_freq);
	else if (attr == &dev_attr_gt_RPn_freq_mhz)
		val = intel_gpu_freq(dev_priv, dev_priv->rps.min_freq);
	else
		BUG();

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static const struct attribute *gen6_attrs[] = {
	&dev_attr_gt_act_freq_mhz.attr,
	&dev_attr_gt_cur_freq_mhz.attr,
	&dev_attr_gt_boost_freq_mhz.attr,
	&dev_attr_gt_max_freq_mhz.attr,
	&dev_attr_gt_min_freq_mhz.attr,
	&dev_attr_gt_RP0_freq_mhz.attr,
	&dev_attr_gt_RP1_freq_mhz.attr,
	&dev_attr_gt_RPn_freq_mhz.attr,
	&dev_attr_kick_delayed_init.attr,
	NULL,
};

static const struct attribute *vlv_attrs[] = {
	&dev_attr_gt_act_freq_mhz.attr,
	&dev_attr_gt_cur_freq_mhz.attr,
	&dev_attr_gt_boost_freq_mhz.attr,
	&dev_attr_gt_max_freq_mhz.attr,
	&dev_attr_gt_min_freq_mhz.attr,
	&dev_attr_gt_RP0_freq_mhz.attr,
	&dev_attr_gt_RP1_freq_mhz.attr,
	&dev_attr_gt_RPn_freq_mhz.attr,
	&dev_attr_vlv_rpe_freq_mhz.attr,
	NULL,
};

#if IS_ENABLED(CONFIG_DRM_I915_CAPTURE_ERROR)

static ssize_t error_state_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr, char *buf,
				loff_t off, size_t count)
{

	struct device *kdev = kobj_to_dev(kobj);
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);
	struct drm_device *dev = &dev_priv->drm;
	struct i915_error_state_file_priv error_priv;
	struct drm_i915_error_state_buf error_str;
	ssize_t ret_count = 0;
	int ret;

	memset(&error_priv, 0, sizeof(error_priv));

	ret = i915_error_state_buf_init(&error_str, to_i915(dev), count, off);
	if (ret)
		return ret;

	error_priv.i915 = dev_priv;
	i915_error_state_get(dev, &error_priv);

	ret = i915_error_state_to_str(&error_str, &error_priv);
	if (ret)
		goto out;

	ret_count = count < error_str.bytes ? count : error_str.bytes;

	memcpy(buf, error_str.buf, ret_count);
out:
	i915_error_state_put(&error_priv);
	i915_error_state_buf_release(&error_str);

	return ret ?: ret_count;
}

static ssize_t gvt_state_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr, char *usr_buf,
				loff_t off, size_t count)
{
	int i, vm_idx;
	ssize_t ret_count = 0;
	static ssize_t total_count;
	cycles_t cur = get_cycles();
	cycles_t start;
	cycles_t idle_total_time = 0;
	static char *buf;

	if (!buf) {
		buf = vzalloc(PAGE_SIZE);
		if (!buf)
			return 0;
	}

	if (off && total_count) {
		ret_count = 0;
		if (off < total_count) {
			ret_count = total_count - off;
			memcpy(usr_buf, &buf[off], total_count - off);
		}
		return ret_count;
	}

	total_count = 0;
	gvt_state.cur_time = cur;
	total_count += sprintf(&buf[total_count],
			"cur_time: %llx\n", gvt_state.cur_time);
	total_count += sprintf(&buf[total_count],
			"eng   cmd#     idl#        idle cycles\n");
	total_count += sprintf(&buf[total_count],
			"================host==================\n");
	for (i = 0; i < GVT_STATE_ENGINES; i++) {
		idle_total_time = gvt_state.host.engines[i].idle_total_time;
		if (atomic_read(&gvt_state.host.engines[i].is_in_idle)) {
			start = gvt_state.host.engines[i].idle_start;
			idle_total_time += cur - start;
		}

		total_count += sprintf(&buf[total_count],
				"e%01d:%8lx#%8lx#%16llx\n",
				i, gvt_state.host.engines[i].submit_count,
				gvt_state.host.engines[i].idle_count,
				idle_total_time);
	}

	for (vm_idx = 0; vm_idx < GVT_STATE_GPUS; vm_idx++) {
		if (!gvt_state.vm[vm_idx].valid)
			continue;

		total_count += sprintf(&buf[total_count],
				"================vgpu%2d================\n",
				vm_idx);
		for (i = 0; i < GVT_STATE_ENGINES; i++) {
			struct engine_idle_state *idle_state =
				&gvt_state.vm[vm_idx].engines[i];
			idle_total_time = idle_state->idle_total_time;
			if (atomic_read(&idle_state->is_in_idle)) {
				start = idle_state->idle_start;
				idle_total_time += cur - start;
			}

			total_count += sprintf(&buf[total_count],
					"e%01d:%8lx#%8lx#%16llx\n",
					i, idle_state->submit_count,
					idle_state->idle_count,
					idle_total_time);
		}
	}

	ret_count = total_count;
	if (count < ret_count)
		ret_count = count;
	memcpy(usr_buf, buf, ret_count);

	return ret_count;
}

static ssize_t error_state_write(struct file *file, struct kobject *kobj,
				 struct bin_attribute *attr, char *buf,
				 loff_t off, size_t count)
{
	struct device *kdev = kobj_to_dev(kobj);
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	DRM_DEBUG_DRIVER("Resetting error state\n");
	i915_destroy_error_state(dev_priv);

	return count;
}

static ssize_t gvt_state_write(struct file *file, struct kobject *kobj,
				 struct bin_attribute *attr, char *buf,
				 loff_t off, size_t count)
{
	gvt_state_reset();
	return count;
}

static struct bin_attribute gvt_state_attr = {
	.attr.name = "gvt_state",
	.attr.mode = S_IRUSR | S_IWUSR,
	.size = 0,
	.read = gvt_state_read,
	.write = gvt_state_write,
};

static struct bin_attribute error_state_attr = {
	.attr.name = "error",
	.attr.mode = S_IRUSR | S_IWUSR,
	.size = 0,
	.read = error_state_read,
	.write = error_state_write,
};

static void i915_setup_error_capture(struct device *kdev)
{
	if (sysfs_create_bin_file(&kdev->kobj, &error_state_attr) ||
		sysfs_create_bin_file(&kdev->kobj, &gvt_state_attr))
		DRM_ERROR("error_state sysfs setup failed\n");
}

static void i915_teardown_error_capture(struct device *kdev)
{
	sysfs_remove_bin_file(&kdev->kobj, &error_state_attr);
}
#else
static void i915_setup_error_capture(struct device *kdev) {}
static void i915_teardown_error_capture(struct device *kdev) {}
#endif

/*
 * DENSO -- force-preempt reset
 */
static ssize_t fpreset_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr, char *buf,
				loff_t off, size_t count)
{
	struct device *kdev = kobj_to_dev(kobj);
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	unsigned long v1;
	long v2;
	char *v3;
	size_t ret;
	struct intel_engine_cs *engine;
	int id;

	intel_forcepreempt_read(dev_priv, &v1, &v2, &v3);

	ret = 0;
	if (!v1) {
		ret += snprintf(buf + ret, count - ret, "0\n");
		return ret;
	}
	ret += snprintf(buf + ret, count - ret,
			"%#lx %ldms %-.16s\n", v1, v2, v3);
	for_each_engine(engine, dev_priv, id) {
		unsigned long mask = intel_engine_flag(engine);
		if (!(v1 & mask))
			continue;
		ret += snprintf(buf + ret, count - ret,
				"%#lx : %s\n",
				mask, engine->name);
	}
	return ret;
}

static ssize_t fpreset_write(struct file *file, struct kobject *kobj,
				 struct bin_attribute *attr, char *buf,
				 loff_t off, size_t count)
{
	struct device *kdev = kobj_to_dev(kobj);
	struct drm_i915_private *dev_priv = kdev_minor_to_i915(kdev);

	char *bp;
	unsigned long v1;	/* v1 -- engine-mask */
	long v2;	/* v2  -- time */
	char v3[sizeof( ((struct task_struct *)0)->comm ) + 1];
			/* v3 -- context name */
	int ret;
	size_t w;
	const char *sep;

	ret = 0;
	if (count && buf[count - 1] == '\n') {
		buf[count - 1] = '\0';
	} else {
		buf[count] = '\0';
	}

	/*
	 * v1 - engine-mask
	 */
	v1 = strtoul(buf, &bp, 0);
	if (bp == buf) {	/* no value */
		ret = -EINVAL;
		goto ret;
	}

	v2 = FORCEPREEMPT_RESET_TIMEOUT;
	v3[0] = '\0';

	/*
	 * v2 - time
	 */
	sep = " ,\t";
	ret = strspn(bp, sep);
	if (!ret) {
		goto doit;
	}
	buf = bp + ret;
	v2 = strtoul(buf, &bp, 10);

	if (*bp == '\n' || *bp == '\0') {
		goto doit;	/* no items */
	}
	if (bp == buf) {	/* invalid char */
		ret = -EINVAL;
		goto ret;
	}

	/*
	 * v3 - context name
	 * using snprintf gains null terminated and size safety
	 * string copy.
	 */
	ret = strspn(bp, sep);
	if (!ret) {
		goto doit;
	}
	bp += ret;
	if (*bp == '\"') {
		/*
		 * quoted
		 */
		buf = bp++;
		sep = "\\\"";
	} else {
		buf = bp;
		sep = "\\ ,\t";
	}
	for (w = 0; *bp; ) {
		int w2;

		w2 = strcspn(bp, sep);
		w += snprintf(v3 + w, sizeof(v3) - w,
				"%.*s", w2, bp);
		// printf("v3=%s w=%ld w2=%d bp=%s\n", v3, w, w2, bp);
		bp += w2;
		if (*bp == '\\' && *++bp) {
			w += snprintf(v3 + w, sizeof(v3) - w,
					"%.1s", bp);
			bp++;
			continue;
		}
		break;
	}
	if (*buf == '\"' && *bp++ != *buf) {
		/*
		 * quoted unmatch
		 */
		ret = -EINVAL;	/* not match quote */
		goto ret;
	}

	// printf("%s %ld %ld <%s>\n", __func__, v1, v2, v3);
	/*
	 * other parameter -- not check more
	 */
doit:
	ret = intel_forcepreempt_write(dev_priv, v1, v2, v3);
ret:
	return ret ?: count;
}

static struct bin_attribute fpreset_attr = {
	.attr.name = "fp_reset",
	.attr.mode = S_IRUSR | S_IWUSR,
	.size = 0,
	.write = fpreset_write,
	.read = fpreset_read,
};

static void i915_setup_fpreset(struct device *kdev)
{
	if (sysfs_create_bin_file(&kdev->kobj, &fpreset_attr)) {
		DRM_ERROR("fpreset sysfs setup failed\n");
	}
}

void i915_setup_sysfs(struct drm_i915_private *dev_priv)
{
	struct device *kdev = dev_priv->drm.primary->kdev;
	int ret;

#ifdef CONFIG_PM
	if (HAS_RC6(dev_priv)) {
		ret = sysfs_merge_group(&kdev->kobj,
					&rc6_attr_group);
		if (ret)
			DRM_ERROR("RC6 residency sysfs setup failed\n");
	}
	if (HAS_RC6p(dev_priv)) {
		ret = sysfs_merge_group(&kdev->kobj,
					&rc6p_attr_group);
		if (ret)
			DRM_ERROR("RC6p residency sysfs setup failed\n");
	}
	if (IS_VALLEYVIEW(dev_priv) || IS_CHERRYVIEW(dev_priv)) {
		ret = sysfs_merge_group(&kdev->kobj,
					&media_rc6_attr_group);
		if (ret)
			DRM_ERROR("Media RC6 residency sysfs setup failed\n");
	}
#endif
	if (HAS_L3_DPF(dev_priv)) {
		ret = device_create_bin_file(kdev, &dpf_attrs);
		if (ret)
			DRM_ERROR("l3 parity sysfs setup failed\n");

		if (NUM_L3_SLICES(dev_priv) > 1) {
			ret = device_create_bin_file(kdev,
						     &dpf_attrs_1);
			if (ret)
				DRM_ERROR("l3 parity slice 1 setup failed\n");
		}
	}

	ret = 0;
	if (IS_VALLEYVIEW(dev_priv) || IS_CHERRYVIEW(dev_priv))
		ret = sysfs_create_files(&kdev->kobj, vlv_attrs);
	else if (INTEL_GEN(dev_priv) >= 6)
		ret = sysfs_create_files(&kdev->kobj, gen6_attrs);
	if (ret)
		DRM_ERROR("RPS sysfs setup failed\n");

	i915_setup_error_capture(kdev);

	if (!intel_vgpu_active(dev_priv)) {
		i915_setup_fpreset(kdev);
	}
}

void i915_teardown_sysfs(struct drm_i915_private *dev_priv)
{
	struct device *kdev = dev_priv->drm.primary->kdev;

	i915_teardown_error_capture(kdev);

	if (IS_VALLEYVIEW(dev_priv) || IS_CHERRYVIEW(dev_priv))
		sysfs_remove_files(&kdev->kobj, vlv_attrs);
	else
		sysfs_remove_files(&kdev->kobj, gen6_attrs);
	device_remove_bin_file(kdev,  &dpf_attrs_1);
	device_remove_bin_file(kdev,  &dpf_attrs);
#ifdef CONFIG_PM
	sysfs_unmerge_group(&kdev->kobj, &rc6_attr_group);
	sysfs_unmerge_group(&kdev->kobj, &rc6p_attr_group);
#endif
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/i915_sysfs.c $ $Rev: 841111 $")
#endif
