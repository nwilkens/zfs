// SPDX-License-Identifier: CDDL-1.0
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
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef	_SYS_ARC_OS_H
#define	_SYS_ARC_OS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * arc_available_memory() returns the number of bytes of memory that the ARC
 * can grow into.  On illumos this is computed from freemem relative to
 * zfs_arc_free_target (the minimum free page count the ARC should leave).
 * A negative return value means memory is already below the free target.
 */
extern int64_t arc_available_memory(void);

/*
 * Minimum number of pages the ARC should leave free.  Initialized from
 * vm_pageout's free target and tunable via /etc/system.
 */
extern uint_t zfs_arc_free_target;

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_ARC_OS_H */
