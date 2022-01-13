#ifndef __LINUX_KVM_PARA_H
#define __LINUX_KVM_PARA_H

#include <uapi/linux/kvm_para.h>


static inline int kvm_para_has_feature(unsigned int feature)
{
	if (kvm_arch_para_features() & (1UL << feature))
		return 1;
	return 0;
}
#endif /* __LINUX_KVM_PARA_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/kvm_para.h $ $Rev: 836322 $")
#endif
