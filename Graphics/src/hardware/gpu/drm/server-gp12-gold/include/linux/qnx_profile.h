#ifndef __QNX_PROFILE_H
#define __QNX_PROFILE_H

#ifdef QNX_PROFILE

#define QNX_PROFILE_FUNC_DEF(func) \
	static inline										\
	int qnx_profile_##func(struct drm_device *dev,		\
						   void *data,					\
						   struct drm_file *file_priv)	\
	{													\
	    int r;											\
	    QNX_PROFILE_BEGIN(#func);			            \
	    r = func(dev, data, file_priv);					\
        QNX_PROFILE_END(#func);							\
        return r;										\
	}


QNX_PROFILE_FUNC_DEF (drm_version);
QNX_PROFILE_FUNC_DEF (drm_getunique);
QNX_PROFILE_FUNC_DEF (drm_getmagic);
QNX_PROFILE_FUNC_DEF (drm_irq_by_busid);
QNX_PROFILE_FUNC_DEF (drm_getmap);
QNX_PROFILE_FUNC_DEF (drm_getclient);
QNX_PROFILE_FUNC_DEF (drm_getstats);
QNX_PROFILE_FUNC_DEF (drm_getcap);
QNX_PROFILE_FUNC_DEF (drm_setversion);
QNX_PROFILE_FUNC_DEF (drm_setunique);
QNX_PROFILE_FUNC_DEF (drm_noop);
QNX_PROFILE_FUNC_DEF (drm_authmagic);
QNX_PROFILE_FUNC_DEF (drm_addmap_ioctl);
QNX_PROFILE_FUNC_DEF (drm_rmmap_ioctl);
QNX_PROFILE_FUNC_DEF (drm_setsareactx);
QNX_PROFILE_FUNC_DEF (drm_getsareactx);
QNX_PROFILE_FUNC_DEF (drm_setmaster_ioctl);
QNX_PROFILE_FUNC_DEF (drm_dropmaster_ioctl);
QNX_PROFILE_FUNC_DEF (drm_addctx);
QNX_PROFILE_FUNC_DEF (drm_rmctx);
QNX_PROFILE_FUNC_DEF (drm_getctx);
QNX_PROFILE_FUNC_DEF (drm_switchctx);
QNX_PROFILE_FUNC_DEF (drm_newctx);
QNX_PROFILE_FUNC_DEF (drm_resctx);
QNX_PROFILE_FUNC_DEF (drm_lock);
QNX_PROFILE_FUNC_DEF (drm_unlock);
QNX_PROFILE_FUNC_DEF (drm_addbufs);
QNX_PROFILE_FUNC_DEF (drm_markbufs);
QNX_PROFILE_FUNC_DEF (drm_infobufs);
QNX_PROFILE_FUNC_DEF (drm_mapbufs);
QNX_PROFILE_FUNC_DEF (drm_freebufs);
QNX_PROFILE_FUNC_DEF (drm_dma_ioctl);
QNX_PROFILE_FUNC_DEF (drm_control);
#if __OS_HAS_AGP
QNX_PROFILE_FUNC_DEF (drm_agp_acquire_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_release_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_enable_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_info_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_alloc_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_free_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_bind_ioctl);
QNX_PROFILE_FUNC_DEF (drm_agp_unbind_ioctl);
#endif

QNX_PROFILE_FUNC_DEF (drm_sg_alloc);
QNX_PROFILE_FUNC_DEF (drm_sg_free);
QNX_PROFILE_FUNC_DEF (drm_wait_vblank);
QNX_PROFILE_FUNC_DEF (drm_modeset_ctl);
QNX_PROFILE_FUNC_DEF (drm_gem_close_ioctl);
QNX_PROFILE_FUNC_DEF (drm_gem_flink_ioctl);
QNX_PROFILE_FUNC_DEF (drm_gem_open_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_getresources);
QNX_PROFILE_FUNC_DEF (drm_prime_handle_to_fd_ioctl);
QNX_PROFILE_FUNC_DEF (drm_prime_fd_to_handle_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_getplane_res);
QNX_PROFILE_FUNC_DEF (drm_mode_getcrtc);
QNX_PROFILE_FUNC_DEF (drm_mode_setcrtc);
QNX_PROFILE_FUNC_DEF (drm_mode_getplane);
QNX_PROFILE_FUNC_DEF (drm_mode_setplane);
QNX_PROFILE_FUNC_DEF (drm_mode_cursor_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_gamma_get_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_gamma_set_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_getencoder);
QNX_PROFILE_FUNC_DEF (drm_mode_getconnector);
QNX_PROFILE_FUNC_DEF (drm_mode_getproperty_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_connector_property_set_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_getblob_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_getfb);
QNX_PROFILE_FUNC_DEF (drm_mode_addfb);
QNX_PROFILE_FUNC_DEF (drm_mode_addfb2);
QNX_PROFILE_FUNC_DEF (drm_mode_rmfb);
QNX_PROFILE_FUNC_DEF (drm_mode_page_flip_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_dirtyfb_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_create_dumb_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_mmap_dumb_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_destroy_dumb_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_obj_get_properties_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_obj_set_property_ioctl);
QNX_PROFILE_FUNC_DEF (drm_mode_cursor2_ioctl);




QNX_PROFILE_FUNC_DEF(i915_dma_init);
QNX_PROFILE_FUNC_DEF(i915_flush_ioctl);
QNX_PROFILE_FUNC_DEF(i915_flip_bufs);
QNX_PROFILE_FUNC_DEF(i915_batchbuffer);
QNX_PROFILE_FUNC_DEF(i915_irq_emit);
QNX_PROFILE_FUNC_DEF( i915_irq_wait);
QNX_PROFILE_FUNC_DEF( i915_getparam);
QNX_PROFILE_FUNC_DEF( i915_setparam);
//QNX_PROFILE_FUNC_DEF( drm_noop);
//QNX_PROFILE_FUNC_DEF( drm_noop);
//QNX_PROFILE_FUNC_DEF( drm_noop);
QNX_PROFILE_FUNC_DEF( i915_cmdbuffer);
//QNX_PROFILE_FUNC_DEF( drm_noop);
//QNX_PROFILE_FUNC_DEF( drm_noop);
QNX_PROFILE_FUNC_DEF( i915_vblank_pipe_get);
QNX_PROFILE_FUNC_DEF( i915_vblank_swap);
QNX_PROFILE_FUNC_DEF( i915_set_status_page);
QNX_PROFILE_FUNC_DEF( i915_gem_init_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_execbuffer);
QNX_PROFILE_FUNC_DEF( i915_gem_execbuffer2);
QNX_PROFILE_FUNC_DEF( i915_gem_pin_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_unpin_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_busy_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_set_caching_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_get_caching_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_throttle_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_entervt_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_leavevt_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_create_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_pread_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_pwrite_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_mmap_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_mmap_gtt_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_set_domain_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_sw_finish_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_set_tiling);
QNX_PROFILE_FUNC_DEF( i915_gem_get_tiling);
QNX_PROFILE_FUNC_DEF( i915_gem_get_aperture_ioctl);
QNX_PROFILE_FUNC_DEF( intel_get_pipe_from_crtc_id);
QNX_PROFILE_FUNC_DEF( i915_gem_madvise_ioctl);
QNX_PROFILE_FUNC_DEF( intel_overlay_put_image);
QNX_PROFILE_FUNC_DEF( intel_overlay_attrs);
QNX_PROFILE_FUNC_DEF( intel_sprite_set_colorkey);
QNX_PROFILE_FUNC_DEF( intel_sprite_get_colorkey);
QNX_PROFILE_FUNC_DEF( i915_gem_wait_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_context_create_ioctl);
QNX_PROFILE_FUNC_DEF( i915_gem_context_destroy_ioctl);
QNX_PROFILE_FUNC_DEF( i915_reg_read_ioctl);

#endif /* QNX_PROFILE */

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/qnx_profile.h $ $Rev: 836322 $")
#endif
