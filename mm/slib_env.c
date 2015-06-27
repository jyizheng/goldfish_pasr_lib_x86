#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/mmzone.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/sort.h>
#include <linux/memblock.h>
#include <linux/highmem.h>
#include <asm/cachetype.h>
#include <asm/pgtable.h>

#include "slib_env.h"
#include "sim.h"
#include "sim-assert.h"

#define DBG printk("%s:%d\n", __func__, __LINE__)

#define ASID(mm)        (0)


char *total_ram = NULL;
struct meminfo meminfo;

extern int memblock_debug;

static void set_memblock_debug(int debug)
{
	memblock_debug = debug;
}

#define TARGET_PHYS_OFFSET 0x00000000

static int memblock_reserve_def(phys_addr_t base, phys_addr_t end)
{
	return memblock_reserve(base - TARGET_PHYS_OFFSET, end - base);
}

static int memblock_free_def(phys_addr_t base, phys_addr_t end)
{
	return memblock_free(base - TARGET_PHYS_OFFSET, end - base);
}

static int memblock_remove_def(phys_addr_t base, phys_addr_t end)
{
	return memblock_remove(base - TARGET_PHYS_OFFSET, end - base);
}

static int memblock_add_def(phys_addr_t base, phys_addr_t end)
{
	return memblock_add(base - TARGET_PHYS_OFFSET, end - base);
}

void *kmap_atomic(struct page *page)
{
	printk("%s:%d\n", __func__, __LINE__);
	return (void *)total_ram + (page_to_pfn(page) << PAGE_SHIFT);
}

void __kunmap_atomic(void *kvaddr)
{
	printk("%s:%d\n", __func__, __LINE__);
}

#ifdef CONFIG_HIGHMEM
static void __init add_one_highpage_init(struct page *page)
{
	ClearPageReserved(page);
	init_page_count(page);
	__free_page(page);
	totalhigh_pages++;
}

void __init add_highpages_with_active_regions(int nid,
			 unsigned long start_pfn, unsigned long end_pfn)
{
	phys_addr_t start, end;
	u64 i;

	for_each_free_mem_range(i, nid, &start, &end, NULL) {
		unsigned long pfn = clamp_t(unsigned long, PFN_UP(start),
					    start_pfn, end_pfn);
		unsigned long e_pfn = clamp_t(unsigned long, PFN_DOWN(end),
					      start_pfn, end_pfn);
		for ( ; pfn < e_pfn; pfn++)
			if (pfn_valid(pfn))
				add_one_highpage_init(pfn_to_page(pfn));
	}
}

void __init set_highmem_pages_init(void)
{
	struct zone *zone;
	int nid;

	for_each_zone(zone) {
		unsigned long zone_start_pfn, zone_end_pfn;

		if (!is_highmem(zone))
			continue;

		zone_start_pfn = zone->zone_start_pfn;
		zone_end_pfn = zone_start_pfn + zone->spanned_pages;

		nid = zone_to_nid(zone);
		printk(KERN_INFO "Initializing %s for node %d (%08lx:%08lx)\n",
				zone->name, nid, zone_start_pfn, zone_end_pfn);

		add_highpages_with_active_regions(nid, zone_start_pfn,
				 zone_end_pfn);
	}
	totalram_pages += totalhigh_pages;
}
#endif

void __init mem_init(void)
{
	memblock_reserve_def(0x0000003677e340, 0x0000003677e487);
	memblock_reserve_def(0x0000003677e1c0, 0x0000003677e307);
	memblock_reserve_def(0x0000003677d1c0, 0x0000003677e1c0);
	memblock_reserve_def(0x00000036775000, 0x0000003677d000);
	memblock_reserve_def(0x0000003677d180, 0x0000003677d184);
	memblock_reserve_def(0x0000003677d140, 0x0000003677d144);
	memblock_reserve_def(0x0000003677d100, 0x0000003677d104);
	memblock_reserve_def(0x0000003677d0c0, 0x0000003677d0c4);
	memblock_reserve_def(0x0000003677d040, 0x0000003677d0b8);
	memblock_reserve_def(0x0000003677d000, 0x0000003677d02c);
	memblock_reserve_def(0x00000036771000, 0x00000036775000);
	memblock_reserve_def(0x000000366f1000, 0x00000036771000);
	memblock_reserve_def(0x000000366b1000, 0x000000366f1000);

#ifdef CONFIG_HIGHMEM
	set_highmem_pages_init();
#endif
	free_all_bootmem();
#ifdef CONFIG_PRINT_BUDDY_FREELIST
	print_buddy_freelist();
#endif
}

static void __init mm_init(void)
{
	mem_init();
}

void __init zone_sizes_init(void)
{
        unsigned long max_zone_pfns[MAX_NR_ZONES];

        memset(max_zone_pfns, 0, sizeof(max_zone_pfns));

	max_low_pfn = 227326;
	max_pfn = 262128;

#ifdef CONFIG_ZONE_DMA
        max_zone_pfns[ZONE_DMA]         = min(MAX_DMA_PFN, max_low_pfn);
#endif
#ifdef CONFIG_ZONE_DMA32
        max_zone_pfns[ZONE_DMA32]       = min(MAX_DMA32_PFN, max_low_pfn);
#endif
        max_zone_pfns[ZONE_NORMAL]      = max_low_pfn;
#ifdef CONFIG_HIGHMEM
        max_zone_pfns[ZONE_HIGHMEM]     = max_pfn;
#endif

        free_area_init_nodes(max_zone_pfns);
}

void __init paging_init(void)
{
	zone_sizes_init();

}

void __init setup_arch(char **cmd)
{
	int ret;
	total_ram = lib_malloc(1024 * 1024 * 1024);
	if (total_ram == NULL)
		printk("Alloc memory failed in %s\n", __func__);

	memblock_reserve_def(0x00000000200000, 0x00000000a44000);
	memblock_reserve_def(0x0000003ff74000, 0x0000003fff0000);
	memblock_reserve_def(0x0000000009f000, 0x00000000100000);
	memblock_reserve_def(0x00000000a44000, 0x00000000a4803d);

	memblock.current_limit = 0x1000000;
	memblock_allow_resize();

	memblock_add_def(0x00000000010000, 0x0000000009f000);
	memblock_add_def(0x00000000100000, 0x0000003fff0000);
	
	memblock_trim_memory(PAGE_SIZE);

	memblock_reserve_def(0x0000000009c000, 0x0000000009f000);
	memblock_reserve_def(0x00000000ff8000, 0x00000000ff9000);

	memblock.current_limit = 0x377fe000;

	memblock_reserve_def(0x00000037782000, 0x000000377fe000);

	memblock_free_def(0x0000003ff74000, 0x0000003fff0000);

	memblock_set_node(0, (phys_addr_t)ULLONG_MAX, 0);

	memblock_reserve_def(0x00000037781000, 0x00000037782000);
	memblock_dump_all();

	paging_init();
	printk("paging init is done\n");

	memblock_reserve_def(0x0000003677e640, 0x0000003677e704);
	memblock_reserve_def(0x0000003677e600, 0x0000003677e640);
	memblock_reserve_def(0x0000003677e5c0, 0x0000003677e600);
	memblock_reserve_def(0x0000003677e580, 0x0000003677e5c0);
	memblock_reserve_def(0x0000003677e540, 0x0000003677e580);
	memblock_reserve_def(0x0000003677e500, 0x0000003677e540);
	memblock_reserve_def(0x0000003677e4c0, 0x0000003677e500);

	printk("setup_arch is done\n");
}

void __init init_memory_system(void)
{
	set_memblock_debug(1);

	setup_arch(NULL);
	build_all_zonelists(NULL);
	page_alloc_init();

	mm_init();

#ifdef CONFIG_PRINT_BUDDY_FREELIST
	print_buddy_freelist();
#endif

}

void test(void)
{
	struct page *page;
	pg_data_t *pgdat = NODE_DATA(nid);
	page = alloc_pages(GFP_KERNEL, 1);

#ifdef CONFIG_PRINT_BUDDY_FREELIST
	//print_buddy_freelist();
#endif
	if (page != NULL)
		__free_pages(page, 1);
	
#ifdef CONFIG_PRINT_BUDDY_FREELIST
	print_buddy_freelist();
#endif

	printk("%s: %p, %p, %d\n", __func__, pgdat->node_zones, 
			pgdat->node_zonelists, pgdat->nr_zones);
	printk("%s:Hello World\n", __FILE__);
}

