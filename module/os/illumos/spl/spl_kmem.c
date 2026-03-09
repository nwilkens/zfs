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
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

/*
 * SPL kernel memory functions for illumos.
 *
 * illumos has native kmem_alloc, kmem_zalloc, kmem_free, kmem_cache_*,
 * kmem_debugging, kmem_size, and kmem_used.  We only need the OpenZFS-
 * specific accessors: spl_kmem_cache_inuse, spl_kmem_cache_entry_size,
 * spl_kmem_cache_set_move, and the kmem_cache_reap_active stub.
 */

#include <sys/zfs_context.h>
#include <sys/kmem.h>
#include <sys/kmem_impl.h>

/*
 * The SPL header redefines kmem_cache_set_move as spl_kmem_cache_set_move.
 * Undefine it so we can call the real illumos function inside our wrapper.
 */
#undef kmem_cache_set_move
extern void kmem_cache_set_move(kmem_cache_t *,
    kmem_cbrc_t (*)(void *, void *, size_t, void *));

/*
 * Return the number of objects currently allocated (in-use) from this cache.
 * buftotal is total buffers allocated to the cache; bufslab is the count
 * of buffers that are free in the slab layer.
 */
uint64_t
spl_kmem_cache_inuse(kmem_cache_t *cache)
{
	return (cache->cache_buftotal - cache->cache_bufslab);
}

/*
 * Return the size of each object in this cache.
 */
uint64_t
spl_kmem_cache_entry_size(kmem_cache_t *cache)
{
	return ((uint64_t)cache->cache_bufsize);
}

/*
 * Register a move callback for cache defragmentation.
 * illumos supports this natively via kmem_cache_set_move().
 */
void
spl_kmem_cache_set_move(kmem_cache_t *skc,
    kmem_cbrc_t (*move)(void *, void *, size_t, void *))
{
	ASSERT3P(move, !=, NULL);
	kmem_cache_set_move(skc, move);
}

/*
 * Check if there are running reaps.
 * On illumos, this is not easily determined; conservatively return B_FALSE.
 */
boolean_t
kmem_cache_reap_active(void)
{
	return (B_FALSE);
}

void
spl_kmem_init(void)
{
	/* Nothing to do; illumos kmem is initialized by the kernel. */
}

void
spl_kmem_fini(void)
{
	/* Nothing to do. */
}
