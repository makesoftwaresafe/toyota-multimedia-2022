#include <linux/qnx.h>
#include <linux/linux.h>

struct class *fb_class;
EXPORT_SYMBOL(fb_class);

/**
 *	fbmem_init - init frame buffer subsystem
 *
 *	Initialize the frame buffer subsystem.
 *
 *	NOTE: This function is _only_ to be called by drivers/char/mem.c.
 *
 */

int fbmem_init(void)
{
	fb_class = class_create(THIS_MODULE, "graphics");
	if (IS_ERR(fb_class)) {
		printk(KERN_WARNING "Unable to create fb class; errno = %ld\n", PTR_ERR(fb_class));
		fb_class = NULL;
	}
	return 0;
}
void
fbmem_exit(void)
{
	//remove_proc_entry("fb", NULL);
	class_destroy(fb_class);
//	unregister_chrdev(FB_MAJOR, "fb");
}

struct fb_info *framebuffer_alloc(size_t size, struct device *dev)
{
	return 0;
}

int fb_alloc_cmap(struct fb_cmap *cmap, int len, int transp)
{
	return 0; //fb_alloc_cmap_gfp(cmap, len, transp, GFP_ATOMIC);
}

int
unregister_framebuffer(struct fb_info *fb_info)
{
	return 0;
}

void framebuffer_release(struct fb_info *info)
{
}

int register_framebuffer(struct fb_info *fb_info)
{
	return 0;
}

void fb_dealloc_cmap(struct fb_cmap *cmap)
{
}

#define VGA_FB_PHYS 0xA0000
extern int remove_conflicting_framebuffers(struct apertures_struct *a, const char *name, bool primary)
{
#if 0
    int i;
    /* check all firmware fbs and kick off if the base addr overlaps */
    for (i = 0 ; i < FB_MAX; i++) {
        struct apertures_struct *gen_aper;
        if (!registered_fb[i])
            continue;

        if (!(registered_fb[i]->flags & FBINFO_MISC_FIRMWARE))
            continue;

        gen_aper = registered_fb[i]->apertures;
        if (fb_do_apertures_overlap(gen_aper, a) ||
            (primary && gen_aper && gen_aper->count &&
             gen_aper->ranges[0].base == VGA_FB_PHYS)) {

            printk(KERN_ERR "fb: conflicting fb hw usage "
                   "%s vs %s - removing generic driver\n",
                   name, registered_fb[i]->fix.id);
            unregister_framebuffer(registered_fb[i]);
        }
    }
#endif
    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/fb.c $ $Rev: 836322 $")
#endif
