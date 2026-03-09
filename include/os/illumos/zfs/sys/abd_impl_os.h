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
 * Copyright (c) 2014 by Chunwei Chen. All rights reserved.
 * Copyright (c) 2016, 2019 by Delphix. All rights reserved.
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef _ABD_IMPL_OS_H
#define	_ABD_IMPL_OS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * On illumos, ABD critical sections are implemented by disabling preemption
 * so that the current CPU cannot be migrated while we operate on scattered
 * page chunks.  kpreempt_disable()/kpreempt_enable() are the appropriate
 * primitives; the flags argument is unused but kept for interface symmetry
 * with the Linux SPL implementation.
 */
#define	abd_enter_critical(flags)	kpreempt_disable()
#define	abd_exit_critical(flags)	kpreempt_enable()

#ifdef __cplusplus
}
#endif

#endif	/* _ABD_IMPL_OS_H */
