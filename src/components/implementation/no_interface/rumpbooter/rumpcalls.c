#include <stdio.h>
#include <string.h>
#include <cos_component.h>
#include <cos_alloc.h>
#include <cos_kernel_api.h>
#include <cos_types.h>

#include "rumpcalls.h"
#include "rump_cos_alloc.h"
#include "cos_sched.h"
//#include "cos_lock.h"
#include "vkern_api.h"
//#define FP_CHECK(void(*a)()) ( (a == null) ? printc("SCHED: ERROR, function pointer is null.>>>>>>>>>>>\n");: printc("nothing");)
#include "cos_sync.h"
#include "micro_booter.h"

extern int vmid;
extern struct cos_compinfo booter_info;
extern struct cos_rumpcalls crcalls;

/* Thread cap */
volatile thdcap_t cos_cur = BOOT_CAPTBL_SELF_INITTHD_BASE;
volatile unsigned int cos_cur_tcap = BOOT_CAPTBL_SELF_INITTCAP_BASE;

tcap_prio_t rk_thd_prio = PRIO_LOW;

/* Mapping the functions from rumpkernel to composite */
void
cos2rump_setup(void)
{
	rump_bmk_memsize_init();

	crcalls.rump_cpu_clock_now 		= cos_cpu_clock_now;
	crcalls.rump_vm_clock_now 		= cos_vm_clock_now;
	crcalls.rump_cos_print 	      		= cos_vm_print;
	crcalls.rump_vsnprintf        		= vsnprintf;
	crcalls.rump_strcmp           		= strcmp;
	crcalls.rump_strncpy          		= strncpy;

	/* These should be removed, confirm that they are never used */
	crcalls.rump_memcalloc        		= cos_memcalloc;
	crcalls.rump_memalloc         		= cos_memalloc;


	crcalls.rump_cos_thdid        		= cos_thdid;
	crcalls.rump_memcpy           		= memcpy;
	crcalls.rump_memset			= cos_memset;
	crcalls.rump_cpu_sched_create 		= cos_cpu_sched_create;

	if(!crcalls.rump_cpu_sched_create) printc("SCHED: rump_cpu_sched_create is set to null");

	crcalls.rump_cpu_sched_switch_viathd    = cos_cpu_sched_switch;
	crcalls.rump_memfree			= cos_memfree;
	crcalls.rump_tls_init 			= cos_tls_init;
	crcalls.rump_va2pa			= cos_vatpa;
	crcalls.rump_pa2va			= cos_pa2va;
	crcalls.rump_resume                     = cos_resume;
	crcalls.rump_platform_exit		= cos_vm_exit;

	crcalls.rump_intr_enable		= intr_enable;
	crcalls.rump_intr_disable		= intr_disable;
	crcalls.rump_sched_yield		= cos_sched_yield;
	crcalls.rump_vm_yield			= cos_vm_yield;

	crcalls.rump_cpu_intr_ack		= cos_cpu_intr_ack;
	crcalls.rump_shmem_send			= cos_shmem_send;
	crcalls.rump_shmem_recv			= cos_shmem_recv;
	crcalls.rump_dequeue_size		= cos_dequeue_size;

	return;
}

#define STR_LEN_MAX 127
static int slen = -1;
static char str[STR_LEN_MAX + 1];

static inline void 
__reset_str(void)
{
	memset(str, 0, STR_LEN_MAX + 1);
	slen = 0;
}

void
cos_printflush(void)
{
	if (slen > 0) {
		cos_print(str, slen);
		__reset_str();
	}
}

/* last few chars still in buffer */
void
cos_vm_print(char s[], int ret)
{
	int len = 0, rem = ret;

	assert(ret <= STR_LEN_MAX+1);
	if (slen == -1) __reset_str();;

	if (slen + rem > STR_LEN_MAX) {
		len = STR_LEN_MAX - slen;
		rem = ret - len;
		strncpy(str+slen, s, len);
		slen += len;
		cos_print(str, slen);

		__reset_str();
	}

	strncpy(str+slen, s+len, rem);
	slen += rem;
	assert(slen <= STR_LEN_MAX);
}

int
cos_dequeue_size(unsigned int srcvm, unsigned int curvm)
{
	return vk_dequeue_size(srcvm, curvm);
}

/*rk shared mem functions*/
int
cos_shmem_send(void * buff, unsigned int size, unsigned int srcvm, unsigned int dstvm){

	asndcap_t sndcap;
	int ret;

	if(srcvm == 0) sndcap = VM0_CAPTBL_SELF_IOASND_SET_BASE + (dstvm - 1) * CAP64B_IDSZ;
	else sndcap = VM_CAPTBL_SELF_IOASND_BASE;

	//printc("%s = s:%d d:%d\n", __func__, srcvm, dstvm);
	cos_shm_write(buff, size, srcvm, dstvm);	

	if(cos_asnd(sndcap, 0)) assert(0);
	return 1;
}

int
cos_shmem_recv(void * buff, unsigned int srcvm, unsigned int curvm){
	//printc("%s = s:%d d:%d\n", __func__, srcvm, curvm);
	return cos_shm_read(buff, srcvm, curvm);
}

/* send and recieve notifications */
void
rump2cos_rcv(void)
{
	printc("rump2cos_rcv");	
	return;
}

void
cos_cpu_intr_ack(void)
{
	static int count = 0;

	if (vmid) return;
	/*
         * ACK interrupts on PIC
         */
        __asm__ __volatile(
            "movb $0x20, %%al\n"
            "outb %%al, $0xa0\n"
            "outb %%al, $0x20\n"
            ::: "al");
	count ++;
//	if (count % 1000 == 0) printc("..%d:ack:%d..", vmid, count);
}

/* irq */
void
cos_irqthd_handler(void *line)
{
	cycles_t first = 0;
	asndcap_t sndcap;
	int which = (int)line;
	arcvcap_t arcvcap = irq_arcvcap[which];
	int prev = 0;

	if (line == 0) sndcap = VM0_CAPTBL_SELF_IOASND_SET_BASE + (DL_VM - 1) * CAP64B_IDSZ;

	while(1) {
		int pending = cos_rcv(arcvcap);

		if ((int)line == 0) {
//			prev++;
//			if (prev % 1000 == 0) printc("--h:%d--", prev);

			if(cos_asnd(sndcap, 0)) assert(0);
		} else {
			intr_start(which);
			bmk_isr(which);
			prev ++;
		//	if (prev % 1000 == 0) printc(" (n:%d) ", prev);
			intr_end();
		}
	}
}

/* Memory */
extern unsigned long bmk_memsize;
void
rump_bmk_memsize_init(void)
{
	/* (1<<20) == 1 MG */
	bmk_memsize = COS_VIRT_MACH_MEM_SZ - ((1<<20)*2);
	printc("FIX ME: ");
	printc("bmk_memsize: %lu\n", bmk_memsize);
}

void
cos_memfree(void *cp)
{
	rump_cos_free(cp);
}

void *
cos_memcalloc(size_t n, size_t size)
{

	printc("cos_memcalloc was called\n");
	while(1);

	void *rv;
	size_t tot = n * size;

	if (size != 0 && tot / size != n)
		return NULL;

	rv = rump_cos_calloc(n, size);
	return rv;
}

void *
cos_memalloc(size_t nbytes, size_t align)
{
	printc("cos_memalloc was called\n");
	while(1);

	void *rv;

	rv = rump_cos_malloc(nbytes);

	return rv;
}

/*---- Scheduling ----*/
int boot_thd = BOOT_CAPTBL_SELF_INITTHD_BASE;

void
cos_tls_init(unsigned long tp, thdcap_t tc)
{
	cos_thd_mod(&booter_info, tc, (void *)tp);
}

void
cos_cpu_sched_create(struct bmk_thread *thread, struct bmk_tcb *tcb,
		void (*f)(void *), void *arg,
		void *stack_base, unsigned long stack_size)
{

	thdcap_t newthd_cap;
	int ret;

	newthd_cap = cos_thd_alloc(&booter_info, booter_info.comp_cap, f, arg);
	assert(newthd_cap);
	set_cos_thddata(thread, newthd_cap, cos_introspect(&booter_info, newthd_cap, 9));
}


static inline void
intr_switch(void)
{
	int ret, i = 32;

	if (!intrs) return;

	/* Man this is ugly...FIXME */
	for (; i > 0 ; i--) {
		int tmp = intrs;

		if ((tmp>>(i-1)) & 1) {
			do {
				ret = cos_switch(irq_thdcap[i], intr_eligible_tcap(i), irq_prio[i], TCAP_TIME_NIL, BOOT_CAPTBL_SELF_INITRCV_BASE, cos_sched_sync());
				assert (ret == 0 || ret == -EAGAIN);
			} while (ret == -EAGAIN);
		}
	}
}

#define CHECK_ITER 32
static inline void
check_vio_budgets(void)
{
	return;
}

static void
cpu_bound_thd_fn(void *d)
{
	cos_thd_switch(BOOT_CAPTBL_SELF_INITTHD_BASE);

	while (1) {
		cos_thd_switch(BOOT_CAPTBL_SELF_INITTHD_BASE);
	}
}

static void
cpu_bound_test(void)
{
	return;
}

static void
print_cycles(void)
{
	static cycles_t total_cycles = 0;
	static cycles_t prev = 0, curr = 0;
	cycles_t cycs_per_sec = cycs_per_usec * 1000 * 1000 * 5;
	tcap_res_t isrbud, viobud, mainbud;

	rdtscll(curr);
	if (prev) total_cycles += (curr - prev);
	prev = curr;

	if (total_cycles >= cycs_per_sec) {
		mainbud = (tcap_res_t)cos_introspect(&booter_info, BOOT_CAPTBL_SELF_INITTCAP_BASE, TCAP_GET_BUDGET);
		printc("vm%d: %lu\n", vmid, mainbud);

		total_cycles = 0;
	}
}

/* Called once from RK init thread. The one in while(1) */
void
cos_resume(void)
{
	static int hpet_irq_blocked = 1;
	static u64_t r1 = 0, r2 = 0, r3 = 0, r4 = 0;
	/* this will not return if this vm is set to be CPU bound */
	//cpu_bound_test();

	while(1) {
		int ret, first = 1;
		unsigned int rk_disabled = 0;
		unsigned int intr_disabled = 0;
		long long until = 0, curr_time = 0;

//		if (r2 % 10000 == 0 || r4 % 10000 == 0) {
//			printc("%d: r1:%llu r2:%llu r3:%llu r4:%llu\n", vmid, r1, r2, r3, r4);
//		}
		r1 ++;
		do {
			unsigned int contending;
			cycles_t cycles;
			int pending, blocked, irq_line;
			thdid_t tid;

			/*
			 * Handle all possible interrupts when
			 * interrupts are enabled or when
			 * a cos interrupt thread has disabled interrupts.
			 * Otherwise a rk thread disabled them and we need to
			 * switch back so it can enable interrupts
			 *
			 * Loop is neccessary incase we get preempted before a valid
			 * interrupt finishes execuing and we requrie that it finishes
			 * executing before returning to RK
		 	 */

			r2 ++;
			do {
				r3 ++;
				int rcvd;
				pending = cos_sched_rcv_all(BOOT_CAPTBL_SELF_INITRCV_BASE, &rcvd, &tid, &blocked, &cycles);
				assert(pending <= 1);

				irq_line = intr_translate_thdid2irq(tid);
				if (irq_line == 0) hpet_irq_blocked = blocked;
				else intr_update(irq_line, blocked);

				if(first) {
					isr_get(cos_isr, &rk_disabled, &intr_disabled, &contending);
					if(rk_disabled && !intr_disabled) goto rk_resume;
					first = 0;
				}
			} while(pending);

			/*
			 * Done processing pending events
			 * Finish any remaining interrupts
			 */
			intr_switch();
			if (!hpet_irq_blocked) {
				do {
					ret = cos_switch(irq_thdcap[0], irq_tcap[0], irq_prio[0], 0, BOOT_CAPTBL_SELF_INITRCV_BASE, cos_sched_sync());
					assert(ret == 0 || ret == -EAGAIN);
				} while (ret == -EAGAIN);
			}
		} while(intrs || !hpet_irq_blocked);

		assert(!intrs && hpet_irq_blocked);

rk_resume:
		/*
		 * Phani: 
		 * Check if BMK Runq has something to RUN!
		 * Return value indicates how long to wait before timing out, 0 if it's not running bmk_platform_block..
		 * This is a check in bmk_platform_block(). 
		 * This upcall should make things faster in idle/block time processing in RK threads.
		 */
		if ((until = bmk_runq_empty())) {
			curr_time = cos_cpu_clock_now();
			if(until > curr_time) {
		//		printc("%d: %lld %lld..", vmid, until, curr_time);
				continue;
			}
		}
		do {
			cycles_t now, then;

			r4 ++;
			if(intr_disabled) break;
			cos_find_vio_tcap();

			rdtscll(then);
			/* TODO: decide which TCAP to use for rest of RK processing for I/O and do deficit accounting */
			ret = cos_switch(cos_cur, COS_CUR_TCAP, rk_thd_prio, TCAP_TIME_NIL, BOOT_CAPTBL_SELF_INITRCV_BASE, cos_sched_sync());
			rdtscll(now);
	//		if (now - then > VM_TIMESLICE * 3 * cycs_per_usec) printc("vm:%d rk:%llu cycs\n", vmid, now - then);
			
			assert(ret == 0 || ret == -EAGAIN);
		} while(ret == -EAGAIN);

		cos_printflush();
		check_vio_budgets();
//		print_cycles();
	}
}

void
cos_cpu_sched_switch(struct bmk_thread *unsused, struct bmk_thread *next)
{
	sched_tok_t tok = cos_sched_sync();
	thdcap_t temp   = get_cos_thdcap(next);
	int ret;

	if(cos_isr) printc("%x\n", (unsigned int)cos_isr);
	assert(!cos_isr);
	cos_cur = temp;

	do {
		ret = cos_switch(cos_cur, COS_CUR_TCAP, rk_thd_prio, TCAP_TIME_NIL, BOOT_CAPTBL_SELF_INITRCV_BASE, tok);
		assert(ret == 0 || ret == -EAGAIN);
		if (ret == -EAGAIN) {
			/*
			 * I was preempted after getting the token and before updating cos_cur which just outdated my sched token
			 * So get a new token and try cos_switch again
			 * 
			 * And cos_cur == temp, can only happen if I've updated cos_cur and there were no other RK threads switched-to after that.
			 */
			if (cos_cur == temp) tok = cos_sched_sync();
			/*
			 * cos_cur is set to 'me' by some other RK thread because I was preempted after updating cos_cur
			 * ignore -EAGAIN in this scenario
			 */
			else break;
		}
	} while (ret == -EAGAIN);
}

/* --------- Timer ----------- */

/* Get the number of cycles in a single micro second. If we do nano second we lose the fraction */
//long long cycles_us = (long long)(CPU_GHZ * 1000);

/* Return monotonic time since RK per VM initiation in nanoseconds */
extern uint64_t t_vm_cycs;
extern uint64_t t_dom_cycs;
long long
cos_vm_clock_now(void)
{
	uint64_t tsc_now = 0;
	unsigned long long curtime = 0;
        
	assert(vmid <= 1);
	if (vmid == 0)      tsc_now = t_dom_cycs;
	else if (vmid == 1) tsc_now = t_vm_cycs;
	
	curtime = (long long)(tsc_now / cycs_per_usec); /* cycles to micro seconds */
        curtime = (long long)(curtime * 1000); /* micro to nano seconds */

	return curtime;
}

/* Return monotonic time since RK initiation in nanoseconds */
long long
cos_cpu_clock_now(void)
{
	uint64_t tsc_now = 0;
	unsigned long long curtime = 0;
        rdtscll(tsc_now);

       	/* We divide as we have cycles and cycles per micro second */
        curtime = (long long)(tsc_now / cycs_per_usec); /* cycles to micro seconds */
        curtime = (long long)(curtime * 1000); /* micro to nano seconds */
      

	return curtime;
}

void *
cos_vatpa(void * vaddr)
{ return cos_va2pa(&booter_info, vaddr); }

void *
cos_pa2va(void * pa, unsigned long len) 
{ return (void *)cos_hw_map(&booter_info, BOOT_CAPTBL_SELF_INITHW_BASE, (paddr_t)pa, (unsigned int)len); }

void
cos_vm_exit(void)
{ while (1) cos_thd_switch(VM_CAPTBL_SELF_EXITTHD_BASE); }

void
cos_sched_yield(void)
{ cos_thd_switch(BOOT_CAPTBL_SELF_INITTHD_BASE); }

void
cos_vm_yield(void)
{ cos_thd_switch(BOOT_CAPTBL_SELF_INITTHD_BASE); }
//{ cos_asnd(VM_CAPTBL_SELF_VKASND_BASE, 1); }

void
cos_dom02io_transfer(unsigned int irqline, tcap_t tc, arcvcap_t rc, tcap_prio_t prio)
{
}

void
cos_vio_tcap_set(unsigned int src)
{
}

void
cos_vio_tcap_update(unsigned int dst)
{
}

tcap_t
cos_find_vio_tcap(void)
{
	return BOOT_CAPTBL_SELF_INITTCAP_BASE;
}
