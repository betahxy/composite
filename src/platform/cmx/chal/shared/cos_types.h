/**
 * Copyright 2014 by Gabriel Parmer, gparmer@gwu.edu
 *
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */

/*
 * Note that this file is included both by the kernel and by
 * components.  Unfortunately, that means that ifdefs might need to be
 * used to maintain the correct defines.
 */

#ifndef TYPES_H
#define TYPES_H

#include "chal/shared/consts.h"
#include "chal/shared/cos_config.h"

#ifndef COS_BASE_TYPES
#define COS_BASE_TYPES
typedef unsigned char      u8_t;
typedef unsigned short int u16_t;
typedef unsigned int       u32_t;
typedef unsigned long long u64_t;
typedef signed char      s8_t;
typedef signed short int s16_t;
typedef signed int       s32_t;
typedef signed long long s64_t;
#endif

#define LLONG_MAX 9223372036854775807LL
typedef u64_t         microsec_t;
typedef u64_t cycles_t;
typedef unsigned long tcap_res_t;
typedef unsigned long tcap_time_t;
typedef u64_t tcap_prio_t;
typedef u64_t tcap_uid_t;
typedef u32_t sched_tok_t;
typedef u32_t word_t;
#define PRINT_CAP_TEMP (1 << 14)
/*
 * The assumption in the following is that cycles_t are higher
 * fidelity than tcap_time_t:
 *
 *  sizeof(cycles_t) >= sizeof(tcap_time_t)
 */
#define TCAP_TIME_QUANTUM_ORD 12
#define TCAP_TIME_MAX_ORD     (TCAP_TIME_QUANTUM_ORD + (sizeof(tcap_time_t) * 8))
#define TCAP_TIME_MAX_BITS(c) ((c >> TCAP_TIME_MAX_ORD) << TCAP_TIME_MAX_ORD)
#define TCAP_TIME_NIL         0
static inline cycles_t
tcap_time2cyc(tcap_time_t c, cycles_t curr)
{ return (((cycles_t)c) << TCAP_TIME_QUANTUM_ORD) | TCAP_TIME_MAX_BITS(curr); }
static inline tcap_time_t
tcap_cyc2time(cycles_t c) {
	tcap_time_t t = (tcap_time_t)(c >> TCAP_TIME_QUANTUM_ORD);
	return t == TCAP_TIME_NIL ? 1 : t;
}
#define CYCLES_DIFF_THRESH (1<<14)
static inline int
cycles_same(cycles_t a, cycles_t b, cycles_t diff_thresh)
{
	return (b < a ? a - b : b - a) <= diff_thresh;
}
/* FIXME: if wraparound happens, we need additional logic to compensate here */
static inline int tcap_time_lessthan(tcap_time_t a, tcap_time_t b) { return a < b; }

typedef enum {
	TCAP_DELEG_TRANSFER = 1,
	TCAP_DELEG_YIELD    = 1<<1,
} tcap_deleg_flags_t;

typedef enum {
	RCV_NON_BLOCKING = 1,
	RCV_ALL_PENDING  = 1 << 1,
} rcv_flags_t;

#define BOOT_LIVENESS_ID_BASE 2

typedef enum {
	CAPTBL_OP_CPY,
	CAPTBL_OP_CONS,
	CAPTBL_OP_DECONS,
	CAPTBL_OP_THDACTIVATE,
	CAPTBL_OP_THDDEACTIVATE,
	CAPTBL_OP_THDTLSSET,
	CAPTBL_OP_COMPACTIVATE,
	CAPTBL_OP_COMPDEACTIVATE,
	CAPTBL_OP_SINVACTIVATE,
	CAPTBL_OP_SINVDEACTIVATE,
	CAPTBL_OP_SRETACTIVATE,
	CAPTBL_OP_SRETDEACTIVATE,
	CAPTBL_OP_ASNDACTIVATE,
	CAPTBL_OP_ASNDDEACTIVATE,
	CAPTBL_OP_ARCVACTIVATE,
	CAPTBL_OP_ARCVDEACTIVATE,
	CAPTBL_OP_MEMACTIVATE,
	CAPTBL_OP_MEMDEACTIVATE,
	/* CAPTBL_OP_MAPPING_MOD, */

	CAPTBL_OP_MEM_RETYPE2USER,
	CAPTBL_OP_MEM_RETYPE2KERN,
	CAPTBL_OP_MEM_RETYPE2FRAME,

	CAPTBL_OP_PGTBLACTIVATE,
	CAPTBL_OP_PGTBLDEACTIVATE,
	CAPTBL_OP_CAPTBLACTIVATE,
	CAPTBL_OP_CAPTBLDEACTIVATE,
	CAPTBL_OP_CAPKMEM_FREEZE,
	CAPTBL_OP_CAPTBLDEACTIVATE_ROOT,
	CAPTBL_OP_PGTBLDEACTIVATE_ROOT,
	CAPTBL_OP_THDDEACTIVATE_ROOT,
	CAPTBL_OP_MEMMOVE,
	CAPTBL_OP_INTROSPECT,
	CAPTBL_OP_TCAP_ACTIVATE,
	CAPTBL_OP_TCAP_TRANSFER,
	CAPTBL_OP_TCAP_DELEGATE,
	CAPTBL_OP_TCAP_MERGE,
	CAPTBL_OP_TCAP_WAKEUP,

	CAPTBL_OP_HW_ACTIVATE,
	CAPTBL_OP_HW_DEACTIVATE,
	CAPTBL_OP_HW_ATTACH,
	CAPTBL_OP_HW_DETACH,
	CAPTBL_OP_HW_MAP,
	CAPTBL_OP_HW_CYC_USEC,
	CAPTBL_OP_HW_CYC_THRESH,
} syscall_op_t;

typedef enum {
	CAP_FREE = 0,
	CAP_SINV,		/* synchronous communication -- invoke */
	CAP_SRET,		/* synchronous communication -- return */
	CAP_ASND,		/* async communication; sender */
	CAP_ARCV,               /* async communication; receiver */
	CAP_THD,                /* thread */
	CAP_COMP,               /* component */
	CAP_CAPTBL,             /* capability table */
	CAP_PGTBL,              /* page-table */
	CAP_FRAME, 		/* untyped frame within a page-table */
	CAP_VM, 		/* mapped virtual memory within a page-table */
	CAP_QUIESCENCE,         /* when deactivating, set to track quiescence state */
	CAP_TCAP, 		/* tcap captable entry */
	CAP_HW,			/* hardware (interrupt) */
} cap_t;

/* TODO: pervasive use of these macros */
/* v \in struct cap_* *, type \in cap_t */
#define CAP_TYPECHK(v, t) ((v) && (v)->h.type == (t))
#define CAP_TYPECHK_CORE(v, type) (CAP_TYPECHK((v), (type)) && (v)->cpuid == get_cpuid())

typedef enum {
	HW_PERIODIC = 32,	/* periodic timer interrupt */
	HW_KEYBOARD,		/* keyboard interrupt */
	HW_ID3,
	HW_ID4,
	HW_SERIAL,		/* serial interrupt */
	HW_ID6,
	HW_ID7,
	HW_ID8,
	HW_ONESHOT,		/* onetime timer interrupt */
	HW_ID10,
	HW_ID11,
	HW_ID12,
	HW_ID13,
	HW_ID14,
	HW_ID15,
	HW_ID16,
	HW_ID17,
	HW_ID18,
	HW_ID19,
	HW_ID20,
	HW_ID21,
	HW_ID22,
	HW_ID23,
	HW_ID24,
	HW_ID25,
	HW_ID26,
	HW_ID27,
	HW_ID28,
	HW_ID29,
	HW_ID30,
	HW_ID31,
	HW_ID32,
	HW_LAPIC_TIMER = 255,  /* Local APIC TSC-DEADLINE mode - Timer interrupts */
} hwid_t;

#define CAP_TYPECHK(v, t) ((v) && (v)->h.type == (t))
#define CAP_TYPECHK_CORE(v, type) (CAP_TYPECHK((v), (type)) && (v)->cpuid == get_cpuid())

typedef unsigned long capid_t;
#define TCAP_PRIO_MAX (1ULL)
#define TCAP_PRIO_MIN ((~0ULL) >> 16) /* 48bit value */
#define TCAP_RES_GRAN_ORD  16
#define TCAP_RES_PACK(r)   (round_up_to_pow2((r), 1 << TCAP_RES_GRAN_ORD))
#define TCAP_RES_EXPAND(r) ((r) << TCAP_RES_GRAN_ORD)
#define TCAP_RES_INF  (~0UL)
#define TCAP_RES_MAX  (TCAP_RES_INF - 1)
#define TCAP_RES_IS_INF(r) (r == TCAP_RES_INF)
typedef capid_t tcap_t;

#define ARCV_NOTIF_DEPTH 8

#define QUIESCENCE_CHECK(curr, past, quiescence_period)  (((curr) - (past)) > (quiescence_period))

/*
 * The values in this enum are the order of the size of the
 * capabilities in this cacheline, offset by CAP_SZ_OFF (to compress
 * memory).
 */
typedef enum {
	CAP_SZ_16B = 0,
	CAP_SZ_32B,
	CAP_SZ_64B,
	CAP_SZ_ERR
} cap_sz_t;
/* the shift offset for the *_SZ_* values */
#define	CAP_SZ_OFF   4
/* The allowed amap bits of each size */
#define	CAP_MASK_16B ((1<<4)-1)
#define	CAP_MASK_32B (1 | (1<<2))
#define	CAP_MASK_64B 1

#define CAP16B_IDSZ (1<<(CAP_SZ_16B))
#define CAP32B_IDSZ (1<<(CAP_SZ_32B))
#define CAP64B_IDSZ (1<<(CAP_SZ_64B))
#define CAPMAX_ENTRY_SZ CAP64B_IDSZ

#define CAPTBL_EXPAND_SZ 128

/* a function instead of a struct to enable inlining + constant prop */
/* PRY:modified the pgtbl size to 256 bytes */
static inline cap_sz_t
__captbl_cap2sz(cap_t c)
{
	/* TODO: optimize for invocation and return */
	switch (c) {
	case CAP_SRET:
	case CAP_THD:
	case CAP_TCAP:
		return CAP_SZ_16B;
	case CAP_SINV:
	case CAP_CAPTBL:
	case CAP_HW: /* TODO: 256bits = 32B * 8b */
		return CAP_SZ_32B;
	case CAP_PGTBL: /* PRY:now page table returns 64B */
	case CAP_COMP:
	case CAP_ASND:
	case CAP_ARCV:
		return CAP_SZ_64B;
	default:
		return CAP_SZ_ERR;
	}
}

static inline unsigned long captbl_idsize(cap_t c)
{ return 1<<__captbl_cap2sz(c); }

/*
 * LLBooter initial captbl setup:
 * 0 = sret,
 * 1-3 = nil,
 * 4-5 = this captbl,
 * 6-7 = our pgtbl root,
 * 8-11 = our component,
 * 12-13 = vm pte for booter
 * 14-15 = untyped memory pgtbl root,
 * 16-17 = vm pte for physical memory,
 * 18-19 = km pte,
 * 20-21 = comp0 captbl,
 * 22-23 = comp0 pgtbl root,
 * 24-27 = comp0 component,
 * 28~(20+2*NCPU) = per core alpha thd

 */

/* Each cap must be aligned to its beginning */
enum {
	BOOT_CAPTBL_SRET               = 0,    //4
	BOOT_CAPTBL_SELF_CT            = 4,    //2
	BOOT_CAPTBL_SELF_PT_META       = 8,    //4
	BOOT_CAPTBL_SELF_PT_TABLE      = 12,   //4
	BOOT_CAPTBL_SELF_COMP          = 16,   //4
	BOOT_CAPTBL_SELF_UNTYPED_PT    = 20,   //4
	BOOT_CAPTBL_SELF_UNTYPED_PT_4K0= 24,   //4
	BOOT_CAPTBL_SELF_UNTYPED_PT_4K1= 28,   //4
	BOOT_CAPTBL_COMP1_CT           = 32,   //2
	BOOT_CAPTBL_COMP2_CT           = 36,   //4
	BOOT_CAPTBL_COMP1_COMP         = 40,   //4
	BOOT_CAPTBL_COMP2_COMP         = 44,
	BOOT_CAPTBL_PARENT_RCV         = 48,
	BOOT_CAPTBL_PARENT_INITTHD     = 52,
	BOOT_CAPTBL_COMP1_INV          = 56,
	BOOT_CAPTBL_COMP2_INV          = 60,
	BOOT_CAPTBL_SELF_INITTHD_BASE  = 64,
	BOOT_CAPTBL_SELF_INITTCAP_BASE = BOOT_CAPTBL_SELF_INITTHD_BASE + NUM_CPU_COS*CAP16B_IDSZ,
	BOOT_CAPTBL_SELF_INITRCV_BASE  = round_up_to_pow2(BOOT_CAPTBL_SELF_INITTCAP_BASE + NUM_CPU_COS*CAP16B_IDSZ, CAPMAX_ENTRY_SZ),
	BOOT_CAPTBL_SELF_INITHW_BASE   = round_up_to_pow2(BOOT_CAPTBL_SELF_INITRCV_BASE + NUM_CPU_COS*CAP64B_IDSZ, CAPMAX_ENTRY_SZ),
	BOOT_CAPTBL_LAST_CAP           = BOOT_CAPTBL_SELF_INITHW_BASE + CAP32B_IDSZ,
	/* round up to next entry */
	BOOT_CAPTBL_FREE               = round_up_to_pow2(BOOT_CAPTBL_LAST_CAP, CAPMAX_ENTRY_SZ)
};

enum {
	BOOT_MEM_VM_BASE = (COS_MEM_COMP_START_VA),
	BOOT_MEM_KM_BASE = (COS_MEM_KERN_START_VA),
};

enum {
	/* thread id */
	THD_GET_TID,
};

enum {
	/* tcap budget */
	TCAP_GET_BUDGET,
};

typedef int cpuid_t; /* Don't use unsigned type. We use negative values for error cases. */

/* Macro used to define per core variables */
#define PERCPU(type, name)                              \
	PERCPU_DECL(type, name);                        \
	PERCPU_VAR(name)

#define PERCPU_DECL(type, name)                         \
struct __##name##_percore_decl {                        \
	type name;                                      \
} CACHE_ALIGNED

#define PERCPU_VAR(name)                                \
struct __##name##_percore_decl name[NUM_CPU]

/* With attribute */
#define PERCPU_ATTR(attr, type, name)	   	        \
	PERCPU_DECL(type, name);                        \
	PERCPU_VAR_ATTR(attr, name)

#define PERCPU_VAR_ATTR(attr, name)                     \
attr struct __##name##_percore_decl name[NUM_CPU]

/* when define an external per cpu variable */
#define PERCPU_EXTERN(name)		                \
	PERCPU_VAR_ATTR(extern, name)

/* We have different functions for getting current CPU in user level
 * and kernel. Thus the GET_CURR_CPU is used here. It's defined
 * separately in user(cos_component.h) and kernel(per_cpu.h).*/
#define PERCPU_GET(name)                (&(name[GET_CURR_CPU].name))
#define PERCPU_GET_TARGET(name, target) (&(name[target].name))

#ifndef NULL
#define NULL ((void*)0)
#endif

/*
 * These types are for addresses that are never meant to be
 * dereferenced.  They will generally be used to set up page table
 * entries.
 */
typedef unsigned long paddr_t;	/* physical address */
typedef unsigned long vaddr_t;	/* virtual address */
typedef unsigned int page_index_t;

typedef unsigned short int spdid_t;
typedef unsigned short int thdid_t;

struct restartable_atomic_sequence {
	vaddr_t start, end;
};

/* see explanation in spd.h */
struct usr_inv_cap {
	vaddr_t invocation_fn, service_entry_inst;
	unsigned int invocation_count, cap_no;
} __attribute__((aligned(16)));

#define COMP_INFO_POLY_NUM 10
#define COMP_INFO_INIT_STR_LEN 128
/* For multicore system, we should have 1 freelist per core. */
#define COMP_INFO_STACK_FREELISTS 1//NUM_CPU_COS

enum {
	COMP_INFO_TMEM_STK = 0,
	COMP_INFO_TMEM_CBUF,
	COMP_INFO_TMEM
};

/* Each stack freelist is associated with a thread id that can be used
 * by the assembly entry routines into a component to decide which
 * freelist to use. */
struct stack_fl {
	vaddr_t freelist;
	unsigned long thd_id;
	char __padding[CACHE_LINE - sizeof(vaddr_t) - sizeof(unsigned long)];
} __attribute__((packed));

struct cos_stack_freelists {
	struct stack_fl freelists[COMP_INFO_STACK_FREELISTS];
};

struct cos_component_information {
	struct cos_stack_freelists cos_stacks;
	long cos_this_spd_id;
	u32_t cos_tmem_relinquish[COMP_INFO_TMEM];
	u32_t cos_tmem_available[COMP_INFO_TMEM];
	vaddr_t cos_heap_ptr, cos_heap_limit;
	vaddr_t cos_heap_allocated, cos_heap_alloc_extent;
	vaddr_t cos_upcall_entry;
	vaddr_t cos_async_inv_entry;
//	struct cos_sched_data_area *cos_sched_data_area;
	vaddr_t cos_user_caps;
	struct restartable_atomic_sequence cos_ras[COS_NUM_ATOMIC_SECTIONS/2];
	vaddr_t cos_poly[COMP_INFO_POLY_NUM];
	char init_string[COMP_INFO_INIT_STR_LEN];
}__attribute__((aligned(PAGE_SIZE)));

typedef enum {
	COS_UPCALL_THD_CREATE,
	COS_UPCALL_ACAP_COMPLETE,
	COS_UPCALL_DESTROY,
	COS_UPCALL_UNHANDLED_FAULT
} upcall_type_t;

enum {
	MAPPING_RO    = 0,
	MAPPING_RW    = 1 << 0,
	MAPPING_KMEM  = 1 << 1
};

/*
 * Fault and fault handler information.  Fault indices/identifiers and
 * the function names to handle them.
 */
typedef enum {
	COS_FLT_PGFLT,
	COS_FLT_DIVZERO,
	COS_FLT_BRKPT,
	COS_FLT_OVERFLOW,
	COS_FLT_RANGE,
	COS_FLT_GEN_PROT,
	/* software defined: */
	COS_FLT_LINUX,
	COS_FLT_SAVE_REGS,
	COS_FLT_FLT_NOTIF,
	COS_FLT_MAX
} cos_flt_off; /* <- this indexes into cos_flt_handlers in the loader */

#define IL_INV_UNMAP (0x1) // when invoking, should we be unmapped?
#define IL_RET_UNMAP (0x2) // when returning, should we unmap?
#define MAX_ISOLATION_LVL_VAL (IL_INV_UNMAP|IL_RET_UNMAP)

/*
 * Note on Symmetric Trust, Symmetric Distruct, and Asym trust:
 * ST  iff (flags & (CAP_INV_UNMAP|CAP_RET_UNMAP) == 0)
 * SDT iff (flags & CAP_INV_UNMAP && flags & CAP_RET_UNMAP)
 * AST iff (!(flags & CAP_INV_UNMAP) && flags & CAP_RET_UNMAP)
 */
#define IL_ST  (0)
#define IL_SDT (IL_INV_UNMAP|IL_RET_UNMAP)
#define IL_AST (IL_RET_UNMAP)
/* invalid type, can NOT be used in data structures, only for return values. */
#define IL_INV (~0)
typedef unsigned int isolation_level_t;


static inline void
cos_mem_fence(void)
{ __asm__ __volatile__("isb" ::: "memory"); }

/* 256 entries. can be increased if necessary */
#define COS_THD_INIT_REGION_SIZE (1<<8)
// Static entries are after the dynamic allocated entries
#define COS_STATIC_THD_ENTRY(i) ((i + COS_THD_INIT_REGION_SIZE + 1))

#ifndef __KERNEL_PERCPU
#define __KERNEL_PERCPU 0
#endif

#endif /* TYPES_H */