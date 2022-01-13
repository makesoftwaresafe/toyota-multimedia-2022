#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/ktime.h>
#include <drm/drmP.h>

void sysfs_notify(struct kobject *kobj, const char *dir, const char *attr)
{
    struct kernfs_node *kn = kobj->sd, *tmp;

    if (kn && dir)
            kn = kernfs_find_and_get(kn, dir);
    else
            kernfs_get(kn);

    if (kn && attr) {
            tmp = kernfs_find_and_get(kn, attr);
            kernfs_put(kn);
            kn = tmp;
    }

    if (kn) {
            kernfs_notify(kn);
            kernfs_put(kn);
    }
}

/* If you want to know what to call your pci_dev, ask this function.
 * Again, it's a wrapper around the generic device.
 */

const char *pci_name(const struct pci_dev *pdev)
{
	const char* name;
	name = dev_name(&pdev->dev);
	if (name == NULL) {
		return "<null-name>";
	}
	return name;
}

/* Simplified asprintf. */
char *kvasprintf(gfp_t gfp, const char *fmt, va_list ap)
{
        unsigned int len;
        char *p;
        va_list aq;

        va_copy(aq, ap);
        len = vsnprintf(NULL, 0, fmt, aq);
        va_end(aq);

        p = kmalloc(len+1, gfp);
        if (!p)
                return NULL;

        vsnprintf(p, len+1, fmt, ap);

        return p;
}

char *kasprintf(gfp_t gfp, const char *fmt, ...)
{
        va_list ap;
        char *p;

        va_start(ap, fmt);
        p = kvasprintf(gfp, fmt, ap);
        va_end(ap);

        return p;
}

void *memdup_user(const void *src, size_t len)
{
    void *p;
    /*
     * Always use GFP_KERNEL, since copy_from_user() can sleep and
     * cause pagefault, which makes it pointless to use GFP_NOFS
     * or GFP_ATOMIC.
     */

    p = kmalloc(len, GFP_KERNEL);
    if (!p)
        return ERR_PTR(-ENOMEM);

    memcpy(p, src, len);

    return p;
}

/**
 * memdup_user_nul - duplicate memory region from user space and NUL-terminate
 *
 * @src: source address in user space
 * @len: number of bytes to copy
 *
 * Returns an ERR_PTR() on failure.
 */
void *memdup_user_nul(const void __user *src, size_t len)
{
	char *p;

	/*
	 * Always use GFP_KERNEL, since copy_from_user() can sleep and
	 * cause pagefault, which makes it pointless to use GFP_NOFS
	 * or GFP_ATOMIC.
	 */
	p = kzalloc(len + 1, GFP_KERNEL);
	if (!p)
		return ERR_PTR(-ENOMEM);

	memcpy(p, src, len);
	p[len] = '\0';

	return p;
}
EXPORT_SYMBOL(memdup_user_nul);

int kstrtoint(const char *s, unsigned int base, int *res)
{
    long long tmp;

    tmp = strtoll(s, NULL, base);

    *res = tmp;
    return 0;
}

char *skip_spaces(const char *str)
{
    while (isspace(*str))
        ++str;
    return (char *)str;
}

void kvfree(void *addr) {
    kfree(addr);
}

void err_printk(const char *msg, const char *s, ...)
{
    va_list va;
    va_start(va, s);
    char tempstr[1024];
    int tempstrlen;

    fprintf(stdout, msg); vfprintf(stdout, s, va); fflush(stdout);
    strncpy(tempstr, msg, sizeof(tempstr)-1);
    strncat(tempstr, s, sizeof(tempstr)-1);
    tempstrlen=strlen(tempstr);
    if ((tempstrlen>0) && (tempstr[tempstrlen-1] == '\n')) {
            tempstr[tempstrlen-1] = 0x00;
    }
    vslogf(SLOGC_SELF, _SLOG_INFO, tempstr, va);
    va_end(va);
}

void printk(const char *s, ...)
{
	char tempstr[1024];
	int tempstrlen;
	va_list va;

	va_start(va, s);
	kvsnprintf(tempstr, sizeof(tempstr) - 1, (*s != KERN_SOH_ASCII) ? s : s + 2, va);
	va_end(va);

	if (drm_debug & DRM_UT_QNX_PRINTF) {
		fprintf(stdout, tempstr); fflush(stdout);
	}
	if (drm_debug & DRM_UT_QNX_SLOGF) {
		tempstrlen = strlen(tempstr);
		if ((tempstrlen>0) && (tempstr[tempstrlen-1] == '\n')) {
			tempstr[tempstrlen-1] = 0x00;
		}
		slogf(SLOGC_SELF, _SLOG_WARNING, tempstr);
	}
	if (drm_debug & DRM_UT_QNX_TRACELOG) {
		trace_nlogf(_NTO_TRACE_USERFIRST+915, 0, tempstr);
	}
}

void vprintk(const char *s, va_list arg)
{
	char tempstr[1024];
	int tempstrlen;

	kvsnprintf(tempstr, sizeof(tempstr) - 1, (*s != KERN_SOH_ASCII) ? s : s + 2, arg);

	if (drm_debug & DRM_UT_QNX_PRINTF) {
		fprintf(stdout, tempstr); fflush(stdout);
	}
	if (drm_debug & DRM_UT_QNX_SLOGF) {
		tempstrlen = strlen(tempstr);
		if ((tempstrlen>0) && (tempstr[tempstrlen-1] == '\n')) {
			tempstr[tempstrlen-1] = 0x00;
		}
		slogf(SLOGC_SELF, _SLOG_WARNING, tempstr);
	}
	if (drm_debug & DRM_UT_QNX_TRACELOG) {
		trace_nlogf(_NTO_TRACE_USERFIRST+915, 0, tempstr);
	}
}

void dev_printk(const char *level, const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, level, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void dev_emerg(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_EMERG, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void dev_alert(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_ALERT, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void dev_crit(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_CRIT, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void dev_err(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_ERR, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void dev_warn(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_WARNING, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void dev_notice(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_NOTICE, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void _dev_info(const struct device *dev, const char *fmt, ...)
{
	char tempstr[1024];
	va_list va;

	strncpy(tempstr, KERN_INFO, sizeof(tempstr) - 1);
	strncat(tempstr, fmt, sizeof(tempstr) - 1 - strlen(tempstr));
	va_start(va, fmt);
	vprintk(tempstr, va);
	va_end(va);
}

void drm_ut_debug_printk(const char *function_name, const char *format, ...)
{
	char tempstr[1024];
	va_list args;

	va_start(args, format);
	sprintf(tempstr, KERN_DEBUG "[" DRM_NAME ":%s] ", function_name);
	strcat(tempstr, format);
	vprintk(tempstr, args);
	va_end(args);
}
EXPORT_SYMBOL(drm_ut_debug_printk);

int wbinvd_on_all_cpus(void)
{
	/* Since we can't implement wbinvd on QNX, we return error. Usually  */
	/* all code tries to use clflush, if it fails it switches to wbinvd. */
	/* So if we are here, we can treat this as bug in our emulation.     */
	BUG();

	return -1;
}

#define KSTRTOX_OVERFLOW	(1U << 31)
const char *_parse_integer_fixup_radix(const char *s, unsigned int *base);
unsigned int _parse_integer(const char *s, unsigned int base, unsigned long long *res);

int _kstrtoul(const char *s, unsigned int base, unsigned long *res)
{
         unsigned long long tmp;
        int rv;
 
         rv = kstrtoull(s, base, &tmp);
         if (rv < 0)
                 return rv;
         if (tmp != (unsigned long long)(unsigned long)tmp)
                 return -ERANGE;
         *res = tmp;
         return 0;
 }

int _kstrtoull(const char *s, unsigned int base, unsigned long long *res)
{
        unsigned long long _res;
        unsigned int rv;

        s = _parse_integer_fixup_radix(s, &base);
        rv = _parse_integer(s, base, &_res);
        if (rv & KSTRTOX_OVERFLOW)
                return -ERANGE;
        if (rv == 0)
                return -EINVAL;
        s += rv;
        if (*s == '\n')
                s++;
        if (*s)
                return -EINVAL;
        *res = _res;
        return 0;
}

int kstrtoull(const char *s, unsigned int base, unsigned long long *res)
{
        if (s[0] == '+')
               s++;
        return _kstrtoull(s, base, res);
}

int kstrtouint(const char *s, unsigned int base, unsigned int *res)
{
        unsigned long long tmp;
        int rv;

        rv = kstrtoull(s, base, &tmp);
        if (rv < 0)
                return rv;
        if (tmp != (unsigned long long)(unsigned int)tmp)
                return -ERANGE;
        *res = tmp;
        return 0;
}

struct notifier_block;
int unregister_oom_notifier(struct notifier_block *nb)
{
	return EOK;
}

/**
 * strreplace - Replace all occurrences of character in string.
 * @s: The string to operate on.
 * @old: The character being replaced.
 * @new: The character @old is replaced with.
 *
 * Returns pointer to the nul byte at the end of @s.
 */
char *strreplace(char *s, char old, char new)
{
	for (; *s; ++s)
		if (*s == old)
			*s = new;
	return s;
}

int intel_scu_ipc_ioread8(u16 addr, u8 *data)
{
    fprintf(stderr, "***ERROR***: intel_scu_ipc_ioread8() is not implemented!\n");
    return -1;
}

int intel_scu_ipc_ioread16(u16 addr, u16 *data)
{
    fprintf(stderr, "***ERROR***: intel_scu_ipc_ioread16() is not implemented!\n");
    return -1;
}

int intel_scu_ipc_ioread32(u16 addr, u32 *data)
{
    fprintf(stderr, "***ERROR***: intel_scu_ipc_ioread32() is not implemented!\n");
    return -1;
}

int intel_scu_ipc_iowrite8(u16 addr, u8 data)
{
    fprintf(stderr, "***ERROR***: intel_scu_ipc_iowrite8() is not implemented!\n");
    return -1;
}

unsigned long get_seconds(void)
{
	unsigned long secs = 0;
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t) == -1 ) {
		perror( "Monolitic clock gettime faied" );
	} else {
		secs = t.tv_sec;
	}
	return secs;
}

inline struct timespec current_kernel_time(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t) == -1 ) {
		perror( "Monolitic clock gettime faied" );
	}
	return t;
}

unsigned int get_random_int(void)
{
	return (unsigned int)mrand48();
}

unsigned long get_random_long(void)
{
	/* When long is 32 bit on x86_32 we don't care about overflow */
	return (unsigned long)(((uint64_t)mrand48()) << 32) | ((uint64_t)mrand48());
}

char* kstrdup(const char *s, gfp_t gfp)
{
	size_t len;
	char *buf;

	if (!s)
		return NULL;

	len = strlen(s) + 1;
	buf = kmalloc(len, gfp);
	if (buf)
		memcpy(buf, s, len);
	return buf;
}

int register_vmap_purge_notifier(struct notifier_block *nb)
{
	return 0;
}

int unregister_vmap_purge_notifier(struct notifier_block *nb)
{
	return 0;
}

/*
 * Create userspace mapping for the DMA-coherent memory.
 */
int dma_common_mmap(struct device *dev, struct vm_area_struct *vma,
		    void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	int ret = -ENXIO;

	BUG();

	return ret;
}
EXPORT_SYMBOL(dma_common_mmap);

pid_t pid_vnr(struct pid *pid)
{
	return pid->pid;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/utils.c $ $Rev: 846821 $")
#endif
