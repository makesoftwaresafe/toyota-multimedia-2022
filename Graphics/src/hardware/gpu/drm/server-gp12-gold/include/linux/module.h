#ifndef _QNX_LINUX_MODULE_H
#define _QNX_LINUX_MODULE_H

#include <linux/moduleparam.h>
#include <linux/export.h>
#include <linux/kmod.h>

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

#define module_init(initfn)								\
	static inline initcall_t __inittest(void)			\
	{ return initfn; }									\
	int qnx_##initfn(void) __attribute__((alias(#initfn)));

//int init_module(void) __attribute__((alias(#initfn)));

/* This is only required if you want to be unloadable. */
#define module_exit(exitfn)								\
	static inline exitcall_t __exittest(void)			\
	{ return exitfn; }									\
	void qnx_##exitfn(void) __attribute__((alias(#exitfn)));

//static inline void cleanup_module(void) { } 


#define MODULE_INFO(tag, info) __MODULE_INFO(tag, tag, info)

#if 0
#define MODULE_AUTHOR(_author) MODULE_INFO(author, _author)
#define MODULE_DESCRIPTION(_description) MODULE_INFO(description, _description)
#define MODULE_LICENSE(_license) MODULE_INFO(license, _license)
#define MODULE_VERSION(_version) MODULE_INFO(license, _version)
#define MODULE_ALIAS(_alias) MODULE_INFO(alias, _alias)
#else
#define MODULE_AUTHOR(_author)
#define MODULE_DESCRIPTION(_description)
#define MODULE_LICENSE(_license)
#define MODULE_VERSION(_version)
#define MODULE_ALIAS(_alias)
#endif

/* Optional firmware file (or files) needed by the module
 * format is simply firmware file name.  Multiple firmware
 * files require multiple MODULE_FIRMWARE() specifiers */
#define MODULE_FIRMWARE(_firmware) MODULE_INFO(firmware, _firmware)


/* TODO. Get/put a kernel symbol (calls should be symmetric) */
#define symbol_get(x) ({ NULL; })
#define symbol_put(x) do { } while (0)
#define symbol_put_addr(x) do { } while (0)

#define postcore_initcall(fn) module_init(fn)
#define postcore_initcall_sync(fn)

static inline bool try_module_get(struct module *module)
{
    return 1;
}

static inline void module_put(struct module *module)
{
}

#define MODULE_DEVICE_TABLE(type, name)

#define MODULE_NAME_LEN MAX_PARAM_PREFIX_LEN

struct module {
	/* Unique handle for this module */
	char name[MODULE_NAME_LEN];

	bool async_probe_requested;
};

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/module.h $ $Rev: 844871 $")
#endif
