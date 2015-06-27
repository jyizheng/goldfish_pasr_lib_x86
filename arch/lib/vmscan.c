/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/mm.h>
#include <linux/shrinker.h>

long vm_total_pages;

/*
 * Add a shrinker callback to be called from the vm
 */
void register_shrinker(struct shrinker *shrinker)
{

}

/*
 * Remove one
 */
void unregister_shrinker(struct shrinker *shrinker)
{
}

