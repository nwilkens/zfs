/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018 by Delphix. All rights reserved.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ARC OS-specific memory management for illumos.
 *
 * illumos provides several kernel memory metrics:
 *   freemem   - number of free physical pages
 *   physmem   - total physical pages
 *   availrmem - available resident memory pages
 *   lotsfree  - high-water mark for pageout scanner
 *   desfree   - desired free memory
 *   needfree  - number of pages the system wants freed
 *   swapfs_minfree - minimum swap space to maintain
 *   pages_pp_maximum - pages that cannot be locked
 */

#include <sys/spa.h>
#include <sys/zio.h>
#include <sys/spa_impl.h>
#include <sys/zio_compress.h>
#include <sys/zio_checksum.h>
#include <sys/zfs_context.h>
#include <sys/arc.h>
#include <sys/arc_os.h>
#include <sys/zfs_refcount.h>
#include <sys/vdev.h>
#include <sys/vdev_trim.h>
#include <sys/vdev_impl.h>
#include <sys/dsl_pool.h>
#include <sys/multilist.h>
#include <sys/abd.h>
#include <sys/zil.h>
#include <sys/fm/fs/zfs.h>
#include <sys/callb.h>
#include <sys/kstat.h>
#include <sys/zthr.h>
#include <sys/arc_impl.h>
#include <sys/sdt.h>
#include <sys/aggsum.h>
#include <sys/vmem.h>

/*
 * illumos kernel memory globals (declared in various kernel headers).
 */
extern pgcnt_t		freemem;
extern pgcnt_t		physmem;
extern pgcnt_t		availrmem;
extern pgcnt_t		lotsfree;
extern pgcnt_t		desfree;
extern pgcnt_t		needfree;
extern pgcnt_t		swapfs_minfree;
extern pgcnt_t		swapfs_reserve;
extern pgcnt_t		pages_pp_maximum;

/*
 * ARC tuning reserves: how much extra headroom to keep
 * beyond the system's built-in thresholds.
 */
static int64_t arc_swapfs_reserve = 64 << 10;	/* pages */
static int64_t arc_pages_pp_reserve = 64 << 10;	/* pages */

/*
 * The zio_arena is a vmem arena used for ZIO data pages
 * on some illumos configurations. When non-NULL, we also
 * track its free space.
 */
extern vmem_t *zio_arena;
extern int arc_zio_arena_free_shift;

/*
 * Return the amount of memory that can be consumed before reclaim will be
 * needed.  Positive if there is sufficient free memory, negative indicates
 * the amount of memory that needs to be freed up.
 */
int64_t
arc_available_memory(void)
{
	int64_t lowest = INT64_MAX;
	int64_t n;

#ifdef _KERNEL
	if (needfree > 0) {
		n = PAGESIZE * (-(int64_t)needfree);
		if (n < lowest)
			lowest = n;
	}

	/*
	 * Check that we're out of range of the pageout scanner.
	 * It starts to schedule paging if freemem is less than
	 * lotsfree + needfree.
	 */
	n = PAGESIZE * ((int64_t)freemem - (int64_t)lotsfree -
	    (int64_t)needfree - (int64_t)desfree);
	if (n < lowest)
		lowest = n;

	/*
	 * Check to make sure that swapfs has enough space so that
	 * anon reservations can still succeed.
	 */
	n = PAGESIZE * ((int64_t)availrmem - (int64_t)swapfs_minfree -
	    (int64_t)swapfs_reserve - (int64_t)desfree -
	    arc_swapfs_reserve);
	if (n < lowest)
		lowest = n;

	/*
	 * Check that we have enough availrmem that memory locking
	 * (e.g., via mlock(3C) or memcntl(2)) can still succeed.
	 */
	n = PAGESIZE * ((int64_t)availrmem - (int64_t)pages_pp_maximum -
	    arc_pages_pp_reserve);
	if (n < lowest)
		lowest = n;

	/*
	 * If zio data pages are being allocated out of a separate
	 * heap segment, check that it has enough free space.
	 */
	if (zio_arena != NULL) {
		n = (int64_t)vmem_size(zio_arena, VMEM_FREE) -
		    (vmem_size(zio_arena, VMEM_ALLOC) >>
		    arc_zio_arena_free_shift);
		if (n < lowest)
			lowest = n;
	}
#endif /* _KERNEL */

	DTRACE_PROBE1(arc__available_memory, int64_t, lowest);
	return (lowest);
}

/*
 * Return a default max arc size based on the amount of physical memory.
 * On illumos, we subtract swapfs_minfree from physmem for the base,
 * and allow 5/8 or (allmem - 1GB), whichever is larger.
 */
uint64_t
arc_default_max(uint64_t min, uint64_t allmem)
{
	uint64_t size;

	if (allmem >= 1ULL << 30)
		size = allmem - (1ULL << 30);
	else
		size = min;

	return (MAX(allmem * 5 / 8, size));
}

/*
 * Return the total amount of physical memory.
 */
uint64_t
arc_all_memory(void)
{
	return (ptob(physmem));
}

/*
 * Return the amount of free physical memory.
 */
uint64_t
arc_free_memory(void)
{
	return (ptob(freemem));
}

/*
 * Throttle memory allocation when memory is tight.
 * On illumos, we cooperate with the pageout daemon.
 */
int
arc_memory_throttle(spa_t *spa, uint64_t reserve, uint64_t txg)
{
#ifdef _KERNEL
	uint64_t available_memory = ptob(freemem);
	static uint_t arc_lotsfree_percent = 10;

	if (freemem > physmem * arc_lotsfree_percent / 100)
		return (0);

	if (txg > spa->spa_lowmem_last_txg) {
		spa->spa_lowmem_last_txg = txg;
		spa->spa_lowmem_page_load = 0;
	}

	/*
	 * If we are in pageout, we know that memory is already tight,
	 * the arc is already going to be evicting, so we just want to
	 * continue to let page writes occur as quickly as possible.
	 */
	if (curproc == proc_pageout) {
		extern pgcnt_t minfree;
		if (spa->spa_lowmem_page_load >
		    MAX(ptob(minfree), available_memory) / 4)
			return (SET_ERROR(ERESTART));
		/* Note: reserve is inflated, so we deflate */
		atomic_add_64(&spa->spa_lowmem_page_load, reserve / 8);
		return (0);
	} else if (spa->spa_lowmem_page_load > 0 && arc_reclaim_needed()) {
		/* memory is low, delay before restarting */
		ARCSTAT_INCR(arcstat_memory_throttle_count, 1);
		return (SET_ERROR(EAGAIN));
	}
	spa->spa_lowmem_page_load = 0;
#else
	(void) spa, (void) reserve, (void) txg;
#endif /* _KERNEL */
	return (0);
}

/*
 * Low-memory event handling.
 * On illumos we use the kernel callback (callb) framework to
 * register for memory pressure notifications.  For now, this is
 * a no-op since the ARC's arc_available_memory() checks are
 * sufficient for memory pressure detection.
 */
void
arc_lowmem_init(void)
{
	/* Nothing needed on illumos -- arc_available_memory() suffices */
}

void
arc_lowmem_fini(void)
{
	/* Nothing to clean up */
}

void
arc_register_hotplug(void)
{
	/* illumos does not support memory hotplug notification */
}

void
arc_unregister_hotplug(void)
{
	/* illumos does not support memory hotplug notification */
}
