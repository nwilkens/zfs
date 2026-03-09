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

#ifndef _SPL_SYS_KMEM_H
#define	_SPL_SYS_KMEM_H

#ifdef _KERNEL
/*
 * illumos provides kmem_alloc, kmem_zalloc, kmem_free, kmem_cache_create,
 * kmem_cache_destroy, kmem_cache_alloc, kmem_cache_free, kmem_cache_reap,
 * KM_SLEEP, KM_NOSLEEP, KM_PUSHPAGE, KM_NORMALPRI natively.
 * Chain to the real header.
 */
#include_next <sys/kmem.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * KMC flags used by OpenZFS that illumos may not define by these names.
 */
#ifndef KMC_NODEBUG
#define	KMC_NODEBUG		0x00000008
#endif

#ifndef KMC_RECLAIMABLE
#define	KMC_RECLAIMABLE		0x00020000
#endif

/*
 * Pointer validity sentinels — used by kmem debugging infrastructure.
 */
#define	POINTER_IS_VALID(p)	(!((uintptr_t)(p) & 0x3))
#define	POINTER_INVALIDATE(pp)	(*(pp) = (void *)((uintptr_t)(*(pp)) | 0x1))

/*
 * freemem and minfree are already exported by illumos as globals.
 * They are declared in <sys/systm.h> on illumos; no redefinition needed.
 */

/*
 * kmem_strfree — free a string allocated via kmem_alloc/kmem_asprintf.
 * OpenZFS uses this everywhere; illumos uses strfree() in
 * usr/src/uts/common/os/strsubr.c.
 */
extern void kmem_strfree(char *str);

/*
 * kmem_strdup — duplicate a string using kmem_alloc.
 */
extern char *kmem_strdup(const char *str);

/*
 * OpenZFS helper functions for formatted string allocation and printing.
 * These are implemented in the OpenZFS illumos port module.
 */
#include <sys/varargs.h>
extern char *kmem_asprintf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));
extern char *kmem_vasprintf(const char *fmt, va_list ap)
    __attribute__((format(printf, 1, 0)));
extern int kmem_scnprintf(char *restrict buf, size_t size,
    const char *restrict fmt, ...)
    __attribute__((format(printf, 3, 4)));

/*
 * spl_kmem_cache_inuse / spl_kmem_cache_entry_size are helper accessors
 * used by OpenZFS ARC/kstat code.
 */
extern uint64_t spl_kmem_cache_inuse(kmem_cache_t *cache);
extern uint64_t spl_kmem_cache_entry_size(kmem_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL */

#endif	/* _SPL_SYS_KMEM_H */
