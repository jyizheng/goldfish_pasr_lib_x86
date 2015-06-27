#ifndef __LINUX_KVM_PARA_H
#define __LINUX_KVM_PARA_H

/*
 * This header file provides a method for making a hypercall to the host
 * Architectures should define:
 * - kvm_hypercall0, kvm_hypercall1...
 * - kvm_arch_para_features
 * - kvm_para_available
 */

/* Return values for hypercalls */
#define KVM_ENOSYS		1000
#define KVM_EFAULT		EFAULT
#define KVM_E2BIG		E2BIG
#define KVM_EPERM		EPERM

#define KVM_HC_VAPIC_POLL_IRQ		1
#define KVM_HC_MMU_OP			2
#define KVM_HC_FEATURES			3
#define KVM_HC_PPC_MAP_MAGIC_PAGE	4

#ifdef CONFIG_PASR_HYPERCALL

#define KVM_HC_PASR_MIN				10

#define KVM_HC_PASR_MM_PAGE_FREE		11
#define KVM_HC_PASR_MM_PAGE_FREE_BATCHED	12
#define KVM_HC_PASR_MM_PAGE_ALLOC		13
#define KVM_HC_PASR_MM_PAGE_ALLOC_ZONE_LOCKED	14
#define KVM_HC_PASR_MM_PAGE_PCPU_DRAIN		15
#define KVM_HC_PASR_MM_PAGE_ALLOC_EXTFRAG	16

#define KVM_HC_PASR_MAX				100
#endif

/*
 * hypercalls use architecture specific
 */
#include <asm/kvm_para.h>

#ifdef __KERNEL__

static inline int kvm_para_has_feature(unsigned int feature)
{
	if (kvm_arch_para_features() & (1UL << feature))
		return 1;
	return 0;
}
#endif /* __KERNEL__ */
#endif /* __LINUX_KVM_PARA_H */
