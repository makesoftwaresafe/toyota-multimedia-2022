/*
 * Copyright (c) 2010 Red Hat Inc.
 * Author : Dave Airlie <airlied@redhat.com>
 *
 * Licensed under GPLv2
 *
 * vga_switcheroo.h - Support for laptop with dual GPU using one set of outputs
 */

#ifndef _LINUX_VGA_SWITCHEROO_H_
#define _LINUX_VGA_SWITCHEROO_H_

#include <linux/fb.h>

struct pci_dev;

/**
 * enum vga_switcheroo_handler_flags_t - handler flags bitmask
 * @VGA_SWITCHEROO_CAN_SWITCH_DDC: whether the handler is able to switch the
 * 	DDC lines separately. This signals to clients that they should call
 * 	drm_get_edid_switcheroo() to probe the EDID
 * @VGA_SWITCHEROO_NEEDS_EDP_CONFIG: whether the handler is unable to switch
 * 	the AUX channel separately. This signals to clients that the active
 * 	GPU needs to train the link and communicate the link parameters to the
 * 	inactive GPU (mediated by vga_switcheroo). The inactive GPU may then
 * 	skip the AUX handshake and set up its output with these pre-calibrated
 * 	values (DisplayPort specification v1.1a, section 2.5.3.3)
 *
 * Handler flags bitmask. Used by handlers to declare their capabilities upon
 * registering with vga_switcheroo.
 */
enum vga_switcheroo_handler_flags_t {
	VGA_SWITCHEROO_CAN_SWITCH_DDC	= (1 << 0),
	VGA_SWITCHEROO_NEEDS_EDP_CONFIG	= (1 << 1),
};

/**
 * enum vga_switcheroo_state - client power state
 * @VGA_SWITCHEROO_OFF: off
 * @VGA_SWITCHEROO_ON: on
 * @VGA_SWITCHEROO_NOT_FOUND: client has not registered with vga_switcheroo.
 * 	Only used in vga_switcheroo_get_client_state() which in turn is only
 * 	called from hda_intel.c
 *
 * Client power state.
 */
enum vga_switcheroo_state {
	VGA_SWITCHEROO_OFF,
	VGA_SWITCHEROO_ON,
	/* below are referred only from vga_switcheroo_get_client_state() */
	VGA_SWITCHEROO_INIT,
	VGA_SWITCHEROO_NOT_FOUND,
};

enum vga_switcheroo_client_id {
	VGA_SWITCHEROO_IGD,
	VGA_SWITCHEROO_DIS,
	VGA_SWITCHEROO_MAX_CLIENTS,
};

/**
 * struct vga_switcheroo_handler - handler callbacks
 * @init: initialize handler.
 * 	Optional. This gets called when vga_switcheroo is enabled, i.e. when
 * 	two vga clients have registered. It allows the handler to perform
 * 	some delayed initialization that depends on the existence of the
 * 	vga clients. Currently only the radeon and amdgpu drivers use this.
 * 	The return value is ignored
 * @switchto: switch outputs to given client.
 * 	Mandatory. For muxless machines this should be a no-op. Returning 0
 * 	denotes success, anything else failure (in which case the switch is
 * 	aborted)
 * @switch_ddc: switch DDC lines to given client.
 * 	Optional. Should return the previous DDC owner on success or a
 * 	negative int on failure
 * @power_state: cut or reinstate power of given client.
 * 	Optional. The return value is ignored
 * @get_client_id: determine if given pci device is integrated or discrete GPU.
 * 	Mandatory
 *
 * Handler callbacks. The multiplexer itself. The @switchto and @get_client_id
 * methods are mandatory, all others may be set to NULL.
 */
struct vga_switcheroo_handler {
	int (*switchto)(enum vga_switcheroo_client_id id);
	int (*switch_ddc)(enum vga_switcheroo_client_id id);
	int (*power_state)(enum vga_switcheroo_client_id id,
			   enum vga_switcheroo_state state);
	int (*init)(void);
	int (*get_client_id)(struct pci_dev *pdev);
};

struct vga_switcheroo_client_ops {
	void (*set_gpu_state)(struct pci_dev *dev, enum vga_switcheroo_state);
	void (*reprobe)(struct pci_dev *dev);
	bool (*can_switch)(struct pci_dev *dev);
};

#if defined(CONFIG_VGA_SWITCHEROO)
void vga_switcheroo_unregister_client(struct pci_dev *dev);
int vga_switcheroo_register_client(struct pci_dev *dev,
				   const struct vga_switcheroo_client_ops *ops,
				   bool driver_power_control);
int vga_switcheroo_register_audio_client(struct pci_dev *pdev,
					 const struct vga_switcheroo_client_ops *ops,
					 int id, bool active);

void vga_switcheroo_client_fb_set(struct pci_dev *dev,
				  struct fb_info *info);

int vga_switcheroo_register_handler(const struct vga_switcheroo_handler *handler,
				    enum vga_switcheroo_handler_flags_t handler_flags);
void vga_switcheroo_unregister_handler(void);
enum vga_switcheroo_handler_flags_t vga_switcheroo_handler_flags(void);
int vga_switcheroo_lock_ddc(struct pci_dev *pdev);
int vga_switcheroo_unlock_ddc(struct pci_dev *pdev);

int vga_switcheroo_process_delayed_switch(void);

int vga_switcheroo_get_client_state(struct pci_dev *dev);

void vga_switcheroo_set_dynamic_switch(struct pci_dev *pdev, enum vga_switcheroo_state dynamic);

int vga_switcheroo_init_domain_pm_ops(struct device *dev, struct dev_pm_domain *domain);
void vga_switcheroo_fini_domain_pm_ops(struct device *dev);
int vga_switcheroo_init_domain_pm_optimus_hdmi_audio(struct device *dev, struct dev_pm_domain *domain);
#else

static inline void vga_switcheroo_unregister_client(struct pci_dev *dev) {}
static inline int vga_switcheroo_register_client(struct pci_dev *dev,
		const struct vga_switcheroo_client_ops *ops, bool driver_power_control) { return 0; }
static inline void vga_switcheroo_client_fb_set(struct pci_dev *dev, struct fb_info *info) {}
static inline int vga_switcheroo_register_handler(const struct vga_switcheroo_handler *handler,
		enum vga_switcheroo_handler_flags_t handler_flags) { return 0; }
static inline int vga_switcheroo_register_audio_client(struct pci_dev *pdev,
	const struct vga_switcheroo_client_ops *ops,
	int id, bool active) { return 0; }
static inline void vga_switcheroo_unregister_handler(void) {}
static inline enum vga_switcheroo_handler_flags_t vga_switcheroo_handler_flags(void) { return 0; }
static inline int vga_switcheroo_lock_ddc(struct pci_dev *pdev) { return -ENODEV; }
static inline int vga_switcheroo_unlock_ddc(struct pci_dev *pdev) { return -ENODEV; }
static inline int vga_switcheroo_process_delayed_switch(void) { return 0; }
static inline int vga_switcheroo_get_client_state(struct pci_dev *dev) { return VGA_SWITCHEROO_ON; }

static inline void vga_switcheroo_set_dynamic_switch(struct pci_dev *pdev, enum vga_switcheroo_state dynamic) {}

static inline int vga_switcheroo_init_domain_pm_ops(struct device *dev, struct dev_pm_domain *domain) { return -EINVAL; }
static inline void vga_switcheroo_fini_domain_pm_ops(struct device *dev) {}
static inline int vga_switcheroo_init_domain_pm_optimus_hdmi_audio(struct device *dev, struct dev_pm_domain *domain) { return -EINVAL; }
static inline bool vga_switcheroo_client_probe_defer(struct pci_dev *pdev) { return false; }

#endif
#endif /* _LINUX_VGA_SWITCHEROO_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/vga_switcheroo.h $ $Rev: 836322 $")
#endif
