#include "kernel.h"
#include "string.h"
#include "mem_layout.h"
#include "pgtbl.h"

struct rsdp {
	char signature[8];
	u8_t checksum;
	char oemid[6];
	u8_t revision;
	u32_t rsdtaddress;
	u32_t length;
	u64_t xsdtaddress;
	u8_t extendedchecksum;
	u8_t reserved[3];
} __attribute__((packed));

struct rsdt {
	char signature[4];
	u32_t length;
	u8_t revision;
	u8_t checksum;
	char oemid[6];
	char oemtableid[8];
	u32_t oemrevision;
	u32_t creatorid;
	u32_t creatorrevision;
	struct rsdt *entry[0];
} __attribute__((packed));

extern u8_t *boot_comp_pgd;
static u32_t basepage;
static struct rsdt *rsdt;

static inline void *
pa2va(void *pa)
{
	return (void*)(((u32_t)pa & ((1<<22)-1)) | basepage);
}

void *
acpi_find_rsdt(void)
{
	unsigned char *sig;
	struct rsdp *rsdp = NULL;
	for (sig = (unsigned char*)0xc00E0000; sig < (unsigned char*)0xc00FFFFF; sig += 16) {
		if (!strncmp("RSD PTR ", (char*)sig, 8)) {
			struct rsdp *r = (struct rsdp*)sig;
			u32_t i;
			unsigned char sum;
			for (i = 0; i < r->length; i++) {
				sum += sig[i];
			}
			if (sum == 0) {
				printk("Found good RSDP @ %p\n", sig);
				rsdp = (struct rsdp*)sig;
				break;
			} else {
				printk("Found RSDP signature but bad checksum (%d) @ %p\n", sum, sig);
			}
		}
	}

	if (rsdp) {
		rsdt = (struct rsdt*)rsdp->rsdtaddress;
	} else {
		rsdt = NULL;
	}

	return rsdt;
}

void *
acpi_find_timer(void)
{
        pgtbl_t pgtbl = (pgtbl_t)boot_comp_pgd;

	size_t i;
	for (i = 0; i < (rsdt->length - sizeof(struct rsdt)) / sizeof(struct rsdt*); i++) {
		struct rsdt *e = (struct rsdt*)pa2va(rsdt->entry[i]);
		if (e->signature[0] == 'H' && e->signature[1] == 'P' &&
			e->signature[2] == 'E' && e->signature[3] == 'T')
		{
			unsigned char *check = (unsigned char*)e;
			unsigned char sum = 0;
			u32_t j;
			for (j = 0; j < e->length; j++) {
				sum += check[j];
			}
			if (sum != 0) {
				printk("Checksum of HPET @ %p failed (got %d)\n", e, sum % 255);
				continue;
			}
			printk("Found good HPET @ %p\n", e);
			return e;
		}
	}

	return NULL;
}


void
acpi_set_rsdt_page(u32_t page)
{
	basepage = page * (1 << 22);
	rsdt = (struct rsdt*)pa2va(rsdt);
}
