#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cos_debug.h>
#include <llprint.h>
#include <cos_component.h>
#include <cos_kernel_api.h>
#include <static_slab.h>
#include <sched.h>
#include <contigmem.h>
#include <shm_bm.h>
#include <vmrt.h>
#include <instr_emul.h>
#include <acrn_common.h>

#include <vlapic.h>
#include <vioapic.h>

INCBIN(vmlinux, "guest/vmlinux.img")
INCBIN(bios, "guest/guest.img")

/* Currently only have one VM component globally managed by this VMM */
static struct vmrt_vm_comp *g_vm;

#define VM_MAX_COMPS (2)
#define GUEST_MEM_SZ (64*1024*1024)

SS_STATIC_SLAB(vm_comp, struct vmrt_vm_comp, VM_MAX_COMPS);
SS_STATIC_SLAB(vm_lapic, struct acrn_vlapic, VM_MAX_COMPS * VMRT_VM_MAX_VCPU);
SS_STATIC_SLAB(vcpu_inst_ctxt, struct instr_emul_ctxt, VM_MAX_COMPS * VMRT_VM_MAX_VCPU);
SS_STATIC_SLAB(vm_io_apic, struct acrn_vioapics, VM_MAX_COMPS);
SS_STATIC_SLAB(vcpu_mmio_req, struct acrn_mmio_request, VM_MAX_COMPS * VMRT_VM_MAX_VCPU);

void
mmio_init(struct vmrt_vm_vcpu *vcpu)
{
	vcpu->mmio_request = ss_vcpu_mmio_req_alloc();
	assert(vcpu->mmio_request);

	ss_vcpu_mmio_req_activate(vcpu->mmio_request);
}

void
ioapic_init(struct vmrt_vm_comp *vm)
{
	vm->ioapic = ss_vm_io_apic_alloc();
	assert(vm->ioapic);

	vioapic_init(vm);
	ss_vm_io_apic_activate(vm->ioapic);
}

void
iinst_ctxt_init(struct vmrt_vm_vcpu *vcpu)
{
	vcpu->inst_ctxt = ss_vcpu_inst_ctxt_alloc();
	assert(vcpu->inst_ctxt);

	ss_vcpu_inst_ctxt_activate(vcpu->inst_ctxt);
}

void
lapic_init(struct vmrt_vm_vcpu *vcpu)
{
	struct acrn_vlapic *vlapic = ss_vm_lapic_alloc();
	assert(vlapic);

	/* A vcpu can only have one vlapic */
	assert(vcpu->vlapic == NULL);

	vcpu->vlapic = vlapic;
	vlapic->apic_page = vcpu->lapic_page;
	vlapic->vcpu = vcpu;

	/* Status: enable APIC and vector be 0xFF */
	vlapic_reset(vlapic);

	ss_vm_lapic_activate(vlapic);
	return;
}

struct vmrt_vm_comp *
vm_comp_create(void)
{
	u64_t guest_mem_sz = GUEST_MEM_SZ;
	u64_t num_vcpu = NUM_CPU;
	void *start;
	void *end;
	cbuf_t shm_id;
	void  *mem, *vm_mem;
	size_t sz;

	struct vmrt_vm_comp *vm = ss_vm_comp_alloc();
	assert(vm);
	
	vmrt_vm_create(vm, "vmlinux-5.15", num_vcpu, guest_mem_sz);

	/* Allocate memory for the VM */
	shm_id	= contigmem_shared_alloc_aligned(guest_mem_sz / PAGE_SIZE_4K, PAGE_SIZE_4K, (vaddr_t *)&mem);
	/* Make the memory accessible to VM */
	memmgr_shared_page_map_aligned_in_vm(shm_id, PAGE_SIZE_4K, (vaddr_t *)&vm_mem, vm->comp_id);
	vmrt_vm_mem_init(vm, mem);
	printc("created VM with %u cpus, memory size: %luMB, at host vaddr: %p\n", vm->num_vcpu, vm->guest_mem_sz/1024/1024, vm->guest_addr);

	ss_vm_comp_activate(vm);

	start = &incbin_bios_start;
	end = &incbin_bios_end;
	sz = end - start + 1;

	printc("BIOS image start: %p, end: %p, size: %lu(%luKB)\n", start, end, sz, sz/1024);
	vmrt_vm_data_copy_to(vm, start, sz, PAGE_SIZE_4K);
	
	start = &incbin_vmlinux_start;
	end = &incbin_vmlinux_end;
	sz = end - start + 1;

	printc("Guest Linux image start: %p, end: %p, size: %lu(%luMB)\n", start, end, sz, sz/1024/1024);
	#define GUEST_IMAGE_ADDR 0x100000
	vmrt_vm_data_copy_to(vm, start, sz, GUEST_IMAGE_ADDR);

	ioapic_init(vm);

	printc("Guest(%s) image has been loaded into the VM component\n", vm->name);

	return vm;
}

void
cos_init(void)
{
	g_vm = vm_comp_create();
}

void
cos_parallel_init(coreid_t cid, int init_core, int ncores)
{
	struct vmrt_vm_vcpu *vcpu;
	vmrt_vm_vcpu_init(g_vm, cid);
	vcpu = vmrt_get_vcpu(g_vm, cid);

	lapic_init(vcpu);
	iinst_ctxt_init(vcpu);
	mmio_init(vcpu);
	return;
}

void
parallel_main(coreid_t cid)
{
	struct vmrt_vm_vcpu *vcpu;
	vcpu = vmrt_get_vcpu(g_vm, cid);

	vmrt_vm_vcpu_start(vcpu);

	while (1)
	{
		sched_thd_block(0);
		/* Should not be here, or there is a bug in the scheduler! */
		assert(0);
	}
}
