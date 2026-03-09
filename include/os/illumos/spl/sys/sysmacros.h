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

#ifndef _SPL_SYS_SYSMACROS_H
#define	_SPL_SYS_SYSMACROS_H

/*
 * illumos provides the full set of system macros natively:
 *   P2ALIGN, P2PHASE, P2NPHASE, P2ROUNDUP, P2END, P2PHASEUP, P2BOUNDARY,
 *   P2SAMEHIGHBIT, IS_P2ALIGNED, ISP2, P2ALIGN_TYPED, P2PHASE_TYPED,
 *   P2NPHASE_TYPED, P2ROUNDUP_TYPED, P2END_TYPED, P2PHASEUP_TYPED,
 *   howmany, roundup, highbit, lowbit, highbit64, lowbit64,
 *   MIN, MAX, ABS, ARRAY_SIZE, DIV_ROUND_UP,
 *   makedevice, getmajor, getminor, DEVCMPL, DEVEXPL, cmpdev, expdev,
 *   kpreempt_disable, kpreempt_enable, CPU_SEQID, is_system_labeled.
 *
 * Chain to the real header.
 */
#include_next <sys/sysmacros.h>
#include <sys/bitmap.h>

/*
 * DIV_ROUND_UP — ceiling integer division.
 * Not native on illumos; defined in Linux and FreeBSD SPL headers.
 */
#ifndef DIV_ROUND_UP
#define	DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))
#endif

/*
 * ____cacheline_aligned — place a struct member or struct on a cache line
 * boundary.  On Linux this comes from <linux/cache.h> using SMP_CACHE_BYTES.
 * On illumos, 64 bytes is the standard cache line size for x86.
 */
#ifndef ____cacheline_aligned
#define	____cacheline_aligned	__attribute__((aligned(64)))
#endif

#endif	/* _SPL_SYS_SYSMACROS_H */
