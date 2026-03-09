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

#ifndef _SPL_SYS_KMEM_CACHE_H
#define	_SPL_SYS_KMEM_CACHE_H

/*
 * On illumos, kmem_cache is part of <sys/kmem.h>; pull it in via our
 * SPL wrapper which also adds the OpenZFS-specific extensions.
 */
#include <sys/kmem.h>

#ifdef _KERNEL

#ifdef __cplusplus
extern "C" {
#endif

/*
 * kmem move callback return values — illumos already defines kmem_cbrc_t
 * in <sys/kmem.h>.  Only define here if not already present.
 */
#ifndef _SYS_KMEM_H
typedef enum kmem_cbrc {
	KMEM_CBRC_YES		= 0,	/* Object moved */
	KMEM_CBRC_NO		= 1,	/* Object not moved */
	KMEM_CBRC_LATER		= 2,	/* Object not moved, try again later */
	KMEM_CBRC_DONT_NEED	= 3,	/* Neither object is needed */
	KMEM_CBRC_DONT_KNOW	= 4,	/* Object unknown */
} kmem_cbrc_t;
#endif

/*
 * kmem_cache_set_move() — register a move callback for cache defragmentation.
 * illumos supports this natively via kmem_cache_move_notify / move callback
 * in kmem_cache_create; provide the SPL shim name used by OpenZFS.
 */
extern void spl_kmem_cache_set_move(kmem_cache_t *,
    kmem_cbrc_t (*)(void *, void *, size_t, void *));

#define	kmem_cache_set_move(cache, move)	\
	spl_kmem_cache_set_move(cache, move)

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL */

#endif	/* _SPL_SYS_KMEM_CACHE_H */
