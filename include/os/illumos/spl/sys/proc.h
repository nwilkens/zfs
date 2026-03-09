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

#ifndef _SPL_SYS_PROC_H
#define	_SPL_SYS_PROC_H

/*
 * illumos provides proc_t, curproc, p0, minclsyspri, maxclsyspri,
 * defclsyspri, thread_create, thread_exit, thread_create_named,
 * CPU, max_ncpus, boot_max_ncpus, TS_RUN, uread, uwrite natively.
 *
 * Chain to the real header.
 */
#include_next <sys/proc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _KERNEL

/*
 * issig — OpenZFS calls issig() with 0 args (Linux style).
 * illumos issig(int why) takes 1 arg (JUSTLOOKING or FORREAL).
 * This cannot be wrapped as a static inline here because issig
 * is declared/defined later. The SPL module provides a 0-arg
 * zfs_issig() wrapper, and we redirect via macro.
 */
#define	issig()		ISSIG(curthread, JUSTLOOKING)

/*
 * zfs_proc_is_caller — returns B_TRUE if the given proc is the caller.
 * Used by OpenZFS to check process identity.
 */
static inline boolean_t
zfs_proc_is_caller(proc_t *p)
{
	return (p == curproc);
}

#endif	/* _KERNEL */

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_PROC_H */
