/*-
 * Copyright (c) 2011 NetApp, Inc.
 * Copyright (c) 2017-2022 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef VLAPIC_H
#define VLAPIC_H

#include <apicreg.h>
#include <acrn_types.h>

/**
 * @file vlapic.h
 *
 * @brief public APIs for virtual LAPIC
 */


#define VLAPIC_MAXLVT_INDEX	APIC_LVT_CMCI

struct hv_timer {
	// struct list_head node;		/**< link all timers */
	int mode;		/**< timer mode: one-shot or periodic */
	uint64_t timeout;		/**< tsc deadline to interrupt */
	uint64_t period_in_cycle;	/**< period of the periodic timer in CPU ticks */
	// timer_handle_t func;		/**< callback if time reached */
	void *priv_data;		/**< func private data */
};

struct vlapic_timer {
	struct hv_timer timer;
	uint32_t mode;
	uint32_t tmicr;
	uint32_t divisor_shift;
};

struct acrn_vlapic {
	/*
	 * Please keep 'apic_page' as the first field in
	 * current structure, as below alignment restrictions are mandatory
	 * to support APICv features:
	 * - 'apic_page' MUST be 4KB aligned.
	 * IRR, TMR and PIR could be accessed by other vCPUs when deliver
	 * an interrupt to vLAPIC.
	 */
	struct lapic_regs	*apic_page;

	struct vmrt_vm_vcpu	*vcpu;

	uint32_t		vapic_id;
	uint32_t		esr_pending;
	int32_t			esr_firing;

	struct vlapic_timer	vtimer;

	/*
	 * isrv: vector number for the highest priority bit that is set in the ISR
	 */
	uint32_t	isrv;

	uint64_t	msr_apicbase;

	const struct acrn_apicv_ops *ops;

	/*
	 * Copies of some registers in the virtual APIC page. We do this for
	 * a couple of different reasons:
	 * - to be able to detect what changed (e.g. svr_last)
	 * - to maintain a coherent snapshot of the register (e.g. lvt_last)
	 */
	uint32_t	svr_last;
	uint32_t	lvt_last[VLAPIC_MAXLVT_INDEX + 1];
} __attribute__((aligned(PAGE_SIZE_4K)));


struct vmrt_vm_vcpu;
struct acrn_apicv_ops {
	void (*accept_intr)(struct acrn_vlapic *vlapic, uint32_t vector, bool level);
	void (*inject_intr)(struct acrn_vlapic *vlapic, bool guest_irq_enabled, bool injected);
	bool (*has_pending_delivery_intr)(struct vmrt_vm_vcpu *vcpu);
	bool (*has_pending_intr)(struct vmrt_vm_vcpu *vcpu);
	bool (*apic_read_access_may_valid)(uint32_t offset);
	bool (*apic_write_access_may_valid)(uint32_t offset);
	bool (*x2apic_read_msr_may_valid)(uint32_t offset);
	bool (*x2apic_write_msr_may_valid)(uint32_t offset);
};

enum reset_mode;
extern const struct acrn_apicv_ops *apicv_ops;
void vlapic_set_apicv_ops(void);

/**
 * @brief virtual LAPIC
 *
 * @addtogroup acrn_vlapic ACRN vLAPIC
 * @{
 */

void vlapic_inject_intr(struct vmrt_vm_vcpu *vcpu);
bool vlapic_has_pending_delivery_intr(struct vmrt_vm_vcpu *vcpu);
bool vlapic_has_pending_intr(struct vmrt_vm_vcpu *vcpu);

/**
 * Returns the highest priority pending vector on vLAPIC, or
 * 0 if there is no pending vector.
 */
uint32_t vlapic_get_next_pending_intr(struct vmrt_vm_vcpu *vcpu);

/**
 * Clears a pending vector from vIRR. Returns true if
 * the bit was previously present, false otherwise.
 */
bool vlapic_clear_pending_intr(struct vmrt_vm_vcpu *vcpu, uint32_t vector);

uint64_t vlapic_get_tsc_deadline_msr(const struct acrn_vlapic *vlapic);
void vlapic_set_tsc_deadline_msr(struct acrn_vlapic *vlapic, uint64_t val_arg);
uint64_t vlapic_get_apicbase(const struct acrn_vlapic *vlapic);
int32_t vlapic_set_apicbase(struct acrn_vlapic *vlapic, uint64_t new);
int32_t vlapic_x2apic_read(struct vmrt_vm_vcpu *vcpu, uint32_t msr, uint64_t *val);
int32_t vlapic_x2apic_write(struct vmrt_vm_vcpu *vcpu, uint32_t msr, uint64_t val);

/*
 * Signals to the LAPIC that an interrupt at 'vector' needs to be generated
 * to the 'cpu', the state is recorded in IRR.
 *  @pre vcpu != NULL
 *  @pre vector <= 255U
 */
void vlapic_set_intr(struct vmrt_vm_vcpu *vcpu, uint32_t vector, bool level);

#define	LAPIC_TRIG_LEVEL	true
#define	LAPIC_TRIG_EDGE		false

static inline uint32_t prio(uint32_t x)
{
	return (x >> 4U);
}

/**
 * @brief Triggers LAPIC local interrupt(LVT).
 *
 * @param[in] vm           Pointer to VM data structure
 * @param[in] vcpu_id_arg  ID of vCPU, BROADCAST_CPU_ID means triggering
 *			   interrupt to all vCPUs.
 * @param[in] lvt_index    The index which LVT would to be fired.
 *
 * @retval 0 on success.
 * @retval -EINVAL on error that vcpu_id_arg or vector of the LVT is invalid.
 *
 * @pre vm != NULL
 */
int32_t vlapic_set_local_intr(struct vmrt_vm_comp *vm, uint16_t vcpu_id_arg, uint32_t lvt_index);

/**
 * @brief Inject MSI to target VM.
 *
 * @param[in] vm   Pointer to VM data structure
 * @param[in] addr MSI address.
 * @param[in] data MSI data.
 *
 * @retval 0 on success.
 * @retval -1 on error that addr is invalid.
 *
 * @pre vm != NULL
 */
int32_t vlapic_inject_msi(struct vmrt_vm_comp *vm, uint64_t addr, uint64_t data);


void vlapic_receive_intr(struct vmrt_vm_comp *vm, bool level, uint32_t dest,
		bool phys, uint32_t delmode, uint32_t vec, bool rh);

/**
 *  @pre vlapic != NULL
 */
static inline uint32_t vlapic_get_apicid(const struct acrn_vlapic *vlapic)
{
	return vlapic->vapic_id;
}

void vlapic_create(struct vmrt_vm_vcpu *vcpu, uint16_t pcpu_id);
/*
 *  @pre vcpu != NULL
 */
void vlapic_free(struct vmrt_vm_vcpu *vcpu);

void vlapic_reset(struct acrn_vlapic *vlapic);
void vlapic_restore(struct acrn_vlapic *vlapic, const struct lapic_regs *regs);
uint64_t vlapic_apicv_get_apic_access_addr(void);
uint64_t vlapic_apicv_get_apic_page_addr(struct acrn_vlapic *vlapic);
int32_t apic_access_vmexit_handler(struct vmrt_vm_vcpu *vcpu);
int32_t apic_write_vmexit_handler(struct vmrt_vm_vcpu *vcpu);
int32_t veoi_vmexit_handler(struct vmrt_vm_vcpu *vcpu);
void vlapic_update_tpr_threshold(const struct acrn_vlapic *vlapic);
int32_t tpr_below_threshold_vmexit_handler(struct vmrt_vm_vcpu *vcpu);
uint64_t vlapic_calc_dest_noshort(struct vmrt_vm_comp *vm, bool is_broadcast,
		uint32_t dest, bool phys, bool lowprio);
bool is_x2apic_enabled(const struct acrn_vlapic *vlapic);
bool is_xapic_enabled(const struct acrn_vlapic *vlapic);

static inline bool is_apicv_advanced_feature_supported(void)
{
	return false;
}
#define POSTED_INTR_ON  0U
/* Posted Interrupt Descriptor (PID) in VT-d spec */
struct pi_desc {
	/* Posted Interrupt Requests, one bit per requested vector */
	uint64_t pir[4];

	union {
		struct {
			/* Outstanding Notification */
			uint16_t on:1;

			/* Suppress Notification, of non-urgent interrupts */
			uint16_t sn:1;

			uint16_t rsvd_1:14;

			/* Notification Vector */
			uint8_t nv;

			uint8_t rsvd_2;

			/* Notification destination, a physical LAPIC ID */
			uint32_t ndst;
		} bits;

		uint64_t value;
	} control;

	uint32_t rsvd[6];
} __attribute__((aligned(64)));;
/**
 * @}
 */
/* End of acrn_vlapic */
#endif /* VLAPIC_H */
