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
	//printk("%s:%d\n", __func__, __LINE__);
	return (void *)total_ram + (page_to_pfn(page) << PAGE_SHIFT);
}

void __kunmap_atomic(void *kvaddr)
{
	//printk("%s:%d\n", __func__, __LINE__);
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

	memblock_reserve_def(0x00000000200000, 0x00000000a63000);
	memblock_reserve_def(0x0000003ff74000, 0x0000003fff0000);
	memblock_reserve_def(0x0000000009f000, 0x00000000100000);
	memblock_reserve_def(0x00000000a63000, 0x00000000a6703d);

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
	init_per_zone_wmark_min();
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

static int per_cpu_pageset_done = 0;

static int kern_trace_parse_alloc(unsigned long order_ul, unsigned long mt_ul,
		char *gfp_masks, unsigned long pfn_ul, unsigned long *allocated_pfn)
{
	struct zonelist *zonelist;
	struct page *pg;
	gfp_t gfp_mask_ul = 0;

	if (strstr(gfp_masks, "GFP_TRANSHUGE")) {
		gfp_mask_ul |= GFP_TRANSHUGE;
	}

	if (strstr(gfp_masks, "GFP_HIGHUSER_MOVABLE")) {
		gfp_mask_ul |= GFP_HIGHUSER_MOVABLE;
	}

	if (strstr(gfp_masks, "GFP_HIGHUSER")) {
		gfp_mask_ul |= GFP_HIGHUSER;
	}

	if (strstr(gfp_masks, "GFP_USER")) {
		gfp_mask_ul |= GFP_USER;
	}

	if (strstr(gfp_masks, "GFP_TEMPORARY")) {
		gfp_mask_ul |= GFP_USER;
	}

	if (strstr(gfp_masks, "GFP_KERNEL")) {
		gfp_mask_ul |= GFP_KERNEL;
	}

	if (strstr(gfp_masks, "GFP_NOFS")) {
		gfp_mask_ul |= GFP_NOFS;
	}

	if (strstr(gfp_masks, "GFP_ATOMIC")) {
		gfp_mask_ul |= GFP_ATOMIC;
	}

	if (strstr(gfp_masks, "GFP_NOIO")) {
		gfp_mask_ul |= GFP_NOIO;
	}

	if (strstr(gfp_masks, "GFP_HIGH")) {
		gfp_mask_ul |= __GFP_HIGH;
	}

	if (strstr(gfp_masks, "GFP_WAIT")) {
		gfp_mask_ul |= __GFP_WAIT;
	}

	if (strstr(gfp_masks, "GFP_IO")) {
		gfp_mask_ul |= __GFP_IO;
	}

	if (strstr(gfp_masks, "GFP_COLD")) {
		gfp_mask_ul |= __GFP_COLD;
	}

	if (strstr(gfp_masks, "GFP_NOWARN")) {
		gfp_mask_ul |= __GFP_NOWARN;
	}

	if (strstr(gfp_masks, "GFP_REPEAT")) {
		gfp_mask_ul |= __GFP_REPEAT;
	}

	if (strstr(gfp_masks, "GFP_NOFAIL")) {
		gfp_mask_ul |= __GFP_NOFAIL;
	}

	if (strstr(gfp_masks, "GFP_NORETRY")) {
		gfp_mask_ul |= __GFP_NORETRY;
	}

	if (strstr(gfp_masks, "GFP_COMP")) {
		gfp_mask_ul |= __GFP_COMP;
	}

	if (strstr(gfp_masks, "GFP_NOMEMALLOC")) {
		gfp_mask_ul |= __GFP_NOMEMALLOC;
	}

	if (strstr(gfp_masks, "GFP_HARDWALL")) {
		gfp_mask_ul |= __GFP_HARDWALL;
	}

	if (strstr(gfp_masks, "GFP_THISNODE")) {
		gfp_mask_ul |= __GFP_THISNODE;
	}

	if (strstr(gfp_masks, "GFP_RECLAIMABLE")) {
		gfp_mask_ul |= __GFP_RECLAIMABLE;
	}

	if (strstr(gfp_masks, "GFP_MOVABLE")) {
		gfp_mask_ul |= __GFP_MOVABLE;
	}

	if (strstr(gfp_masks, "GFP_NOTRACK")) {
		gfp_mask_ul |= __GFP_NOTRACK;
	}

	if (strstr(gfp_masks, "GFP_NO_KSWAPD")) {
		gfp_mask_ul |= __GFP_NO_KSWAPD;
	}

	if (strstr(gfp_masks, "GFP_OTHER_NODE")) {
		gfp_mask_ul |= __GFP_OTHER_NODE;
	}

	if (strstr(gfp_masks, "GFP_NOWAIT")) {
		gfp_mask_ul |= GFP_NOWAIT;
	}

	if (strstr(gfp_masks, "0x2")) {
		gfp_mask_ul |= 0x2;
	}

	if (strstr(gfp_masks, "0x1000002")) {
		gfp_mask_ul |= 0x1000002;
	}

	if (strstr(gfp_masks, "GFP_ZERO")) {
		gfp_mask_ul |= __GFP_ZERO;
	}

	zonelist = NODE_DATA(0)->node_zonelists + 0;
	pg = __alloc_pages_nodemask(gfp_mask_ul, order_ul, zonelist, NULL);

out:
	if (pg == NULL)
		printk("Error\n");

	*allocated_pfn = page_to_pfn(pg);
	printk("alloc: %lu trace: %lu, %s\n", *allocated_pfn, pfn_ul,
			page_to_pfn(pg) == pfn_ul ? "True" : "False");
#if 0
	if (page_to_pfn(pg) != pfn_ul) {
		print_buddy_freelist();
		print_zone_pageset();
		return -1;
	}
#endif
	if (!per_cpu_pageset_done && order_ul == 3) {
		setup_per_cpu_pageset();
		per_cpu_pageset_done = 1;
	}
	return 0;
}

int kern_trace_parse_alloc_one(char *order, char *mt, char *gfp_masks, char *pfn,
	unsigned long *allocated_pfn)
{
	unsigned long order_ul;
	unsigned long mt_ul;
	unsigned long pfn_ul;

	order[-1] = '\0';
	kstrtoul(pfn + 4, 10, &pfn_ul);

	mt[-1] = '\0';
	kstrtoul(order + 6, 10, &order_ul);

	gfp_masks[-1] = '\0';
	kstrtoul(mt + 12, 10, &mt_ul);

	if (order_ul == 0)
		return kern_trace_parse_alloc(order_ul, mt_ul, gfp_masks, pfn_ul, allocated_pfn);

	return 0;
}

int kern_trace_parse_alloc_lock(char *order, char *mt, char *gfp_masks, char *pfn,
	unsigned long *allocated_pfn)
{
	unsigned long order_ul;
	unsigned long mt_ul;
	unsigned long pfn_ul;
	int ret;

	order[-1] = '\0';
	kstrtoul(pfn + 4, 10, &pfn_ul);

	mt[-1] = '\0';
	kstrtoul(order + 6, 10, &order_ul);

	kstrtoul(mt + 12, 10, &mt_ul);

	ret = kern_trace_parse_alloc(order_ul, mt_ul, gfp_masks, pfn_ul, allocated_pfn);
	return ret;
}


int kern_trace_parse_free(char *order, unsigned long pfn, int cold)
{
	unsigned long order_ul;
	struct page *page;
	
	kstrtoul(order + 6, 10, &order_ul);
	printk("free: order:%lu, pfn=%lu\n", order_ul, pfn);

	page = pfn_to_page(pfn);

	if (cold == 0)
		__free_pages(page, order_ul);
	else if (order_ul == 0)
		free_hot_cold_page(page, cold);
	else {
		printk("For cold page, order is 0\n");
		return -1;
	}
	return 0;
}
