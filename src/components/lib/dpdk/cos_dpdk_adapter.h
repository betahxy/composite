#ifndef COS_DPDK_ADAPTER_H
#define COS_DPDK_ADAPTER_H

#define DEFINE_COS_PMD(pmd_name) char COS_##pmd_name##_PMD;
#define DECLARE_COS_PMD(pmd_name) extern char COS_##pmd_name##_PMD; char *COS_##pmd_name##_PMD_PTR = &COS_##pmd_name##_PMD;

int cos_printc(const char *fmt,va_list ap);
int cos_printf(const char *fmt,...);

int cos_bus_scan(void);
int cos_pci_scan(void);

void *cos_map_phys_to_virt(void *paddr, unsigned int size);
void *cos_map_virt_to_phys(void *addr);

int cos_dpdk_init(int argc, char **argv);
#endif /* COS_DPDK_ADAPTER_H */