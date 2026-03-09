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
 * Copyright (c) 2024, OpenZFS Contributors.  All rights reserved.
 */

#ifndef _SPL_SYS_ATOMIC_H
#define	_SPL_SYS_ATOMIC_H

/*
 * illumos provides all atomic operations natively:
 *   atomic_add_8/16/32/64, atomic_sub_*, atomic_inc_*, atomic_dec_*,
 *   atomic_cas_8/16/32/64/ptr, atomic_swap_*, atomic_add_*_nv,
 *   membar_consumer, membar_producer, membar_sync, membar_enter, membar_exit.
 *
 * Simply chain to the real header.
 */
#include_next <sys/atomic.h>

/*
 * atomic_load_64 — atomic 64-bit load.
 * Not native on illumos; used by OpenZFS refcount code.
 */
#ifndef atomic_load_32
static inline uint32_t
atomic_load_32(volatile uint32_t *target)
{
	return (__atomic_load_n(target, __ATOMIC_RELAXED));
}
#endif

#ifndef atomic_load_64
static inline uint64_t
atomic_load_64(volatile uint64_t *target)
{
	return (__atomic_load_n(target, __ATOMIC_RELAXED));
}
#endif

#ifndef atomic_store_64
static inline void
atomic_store_64(volatile uint64_t *target, uint64_t val)
{
	__atomic_store_n(target, val, __ATOMIC_RELAXED);
}
#endif

#ifndef atomic_sub_64
static inline void
atomic_sub_64(volatile uint64_t *target, int64_t delta)
{
	(void) atomic_add_64_nv(target, -delta);
}
#endif

static inline void *
atomic_load_ptr(volatile void *target)
{
	return ((void *)atomic_load_64((volatile uint64_t *)target));
}

#endif	/* _SPL_SYS_ATOMIC_H */
