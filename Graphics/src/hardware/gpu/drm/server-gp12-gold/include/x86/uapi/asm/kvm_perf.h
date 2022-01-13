#ifndef _ASM_X86_KVM_PERF_H
#define _ASM_X86_KVM_PERF_H

#include <asm/svm.h>
#include <asm/vmx.h>
#include <asm/kvm.h>

#define DECODE_STR_LEN 20

#define VCPU_ID "vcpu_id"

#define KVM_ENTRY_TRACE "kvm:kvm_entry"
#define KVM_EXIT_TRACE "kvm:kvm_exit"
#define KVM_EXIT_REASON "exit_reason"

#endif /* _ASM_X86_KVM_PERF_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/kvm_perf.h $ $Rev: 836322 $")
#endif
