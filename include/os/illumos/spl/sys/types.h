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

#ifndef _SPL_SYS_TYPES_H
#define	_SPL_SYS_TYPES_H

/*
 * illumos IS the native platform; chain to the real system header.
 * illumos already provides: uint_t, boolean_t, uchar_t, ushort_t, ulong_t,
 * u_longlong_t, longlong_t, offset_t, major_t, minor_t, id_t, pgcnt_t,
 * taskid_t, projid_t, poolid_t, zoneid_t, ctid_t, diskaddr_t, timestruc_t,
 * hrtime_t, and many more.
 */
#include_next <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OpenZFS-specific type aliases not defined by illumos natively.
 */

/* Used for kernel module parameters (opaque on illumos). */
typedef void		zfs_kernel_param_t;

/*
 * inode timestamp type — illumos timestruc_t is struct timespec.
 * Use struct timespec directly to avoid include ordering issues.
 */
typedef struct timespec	inode_timespec_t;

/* umode_t — Linux-ism for file permission mode. Map to illumos mode_t. */
typedef mode_t		umode_t;

/* ID-mapping opaque type (not used on illumos, kept for portability). */
typedef void		zidmap_t;

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_TYPES_H */
