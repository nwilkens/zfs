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

#ifndef _SPL_SYS_SIMD_H
#define	_SPL_SYS_SIMD_H

/*
 * SIMD dispatch for illumos.
 *
 * illumos runs on x86/x86-64.  The kernel FPU save/restore facility is
 * provided by <sys/kfpu.h>.  For non-x86 architectures (which illumos
 * does not use in practice) we fall back to the stub implementations.
 */

#if defined(__amd64__) || defined(__i386__)
#include <sys/simd_x86.h>

#else

#define	kfpu_allowed()		0
#define	kfpu_initialize(tsk)	do {} while (0)
#define	kfpu_begin()		do {} while (0)
#define	kfpu_end()		do {} while (0)
#define	kfpu_init()		(0)
#define	kfpu_fini()		do {} while (0)

#endif	/* __amd64__ || __i386__ */

#define	simd_stat_init()	do {} while (0)
#define	simd_stat_fini()	do {} while (0)

#endif	/* _SPL_SYS_SIMD_H */
