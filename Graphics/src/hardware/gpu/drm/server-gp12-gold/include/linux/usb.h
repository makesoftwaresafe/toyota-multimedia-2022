#ifndef _QNX_LINUX_USB_H
#define _QNX_LINUX_USB_H

#if 0
struct usb_device {
	int		devnum;
	char		devpath[16];
	u32		route;
	enum usb_device_state	state;
	enum usb_device_speed	speed;

	struct usb_tt	*tt;
	int		ttport;

	unsigned int toggle[2];

	struct usb_device *parent;
	struct usb_bus *bus;
	struct usb_host_endpoint ep0;

	struct device dev;

	struct usb_device_descriptor descriptor;
	struct usb_host_bos *bos;
	struct usb_host_config *config;

	struct usb_host_config *actconfig;
	struct usb_host_endpoint *ep_in[16];
	struct usb_host_endpoint *ep_out[16];

	char **rawdescriptors;

	unsigned short bus_mA;
	u8 portnum;
	u8 level;

	unsigned can_submit:1;
	unsigned persist_enabled:1;
	unsigned have_langid:1;
	unsigned authorized:1;
	unsigned authenticated:1;
	unsigned wusb:1;
	unsigned lpm_capable:1;
	unsigned usb2_hw_lpm_capable:1;
	unsigned usb2_hw_lpm_besl_capable:1;
	unsigned usb2_hw_lpm_enabled:1;
	unsigned usb2_hw_lpm_allowed:1;
	unsigned usb3_lpm_enabled:1;
	int string_langid;

	/* static strings from the device */
	char *product;
	char *manufacturer;
	char *serial;

	struct list_head filelist;

	int maxchild;

	u32 quirks;
	atomic_t urbnum;

	unsigned long active_duration;

#ifdef CONFIG_PM
	unsigned long connect_time;

	unsigned do_remote_wakeup:1;
	unsigned reset_resume:1;
	unsigned port_is_suspended:1;
#endif
	struct wusb_dev *wusb_dev;
	int slot_id;
	enum usb_device_removable removable;
	struct usb2_lpm_parameters l1_params;
	struct usb3_lpm_parameters u1_params;
	struct usb3_lpm_parameters u2_params;
	unsigned lpm_disable_count;
};

#else

struct usb_device {};
struct usb_device_id;

#endif

struct usbdrv_wrap {
	struct device_driver driver;
	int for_devices;
};
struct usb_device_driver {
	const char *name;

	int (*probe) (struct usb_device *udev);
	void (*disconnect) (struct usb_device *udev);

	int (*suspend) (struct usb_device *udev, pm_message_t message);
	int (*resume) (struct usb_device *udev, pm_message_t message);
	struct usbdrv_wrap drvwrap;
	unsigned int supports_autosuspend:1;
};

#define	to_usb_device(d) container_of(d, struct usb_device, dev)

#define usb_register(driver) \
	usb_register_driver(driver/*, THIS_MODULE, KBUILD_MODNAME*/)

struct usb_driver;
static inline void
usb_deregister(struct usb_driver *drv){
	//TODO.
}

static inline int
usb_register_driver(struct usb_driver *drv){
	//, struct module *mod,
	//	const char *name){
	//TODO.
	return 0;
}

struct usb_interface;
static inline struct usb_device *
interface_to_usbdev(struct usb_interface *intf)
{
	return 0; //to_usb_device(intf->dev.parent);
}


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/usb.h $ $Rev: 836322 $")
#endif
