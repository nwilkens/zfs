// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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

#ifndef _SPL_SYS_VMSYSTM_H
#define	_SPL_SYS_VMSYSTM_H

/*
 * illumos VM system globals.
 *
 * freemem   - number of free pages available in the system
 * availrmem - number of pages available for user processes
 *
 * These are declared in <sys/systm.h> or <vm/vm_dep.h> on illumos.
 */
#include <sys/systm.h>

#ifdef _KERNEL
extern pgcnt_t	freemem;
extern pgcnt_t	availrmem;
#endif

#endif	/* _SPL_SYS_VMSYSTM_H */
