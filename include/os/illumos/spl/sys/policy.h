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

#ifndef _SPL_SYS_POLICY_H
#define	_SPL_SYS_POLICY_H

/*
 * illumos has native security policy functions: secpolicy_fs_mount(),
 * secpolicy_vnode_setattr(), secpolicy_zfs(), etc.
 */
#include_next <sys/policy.h>

/*
 * secpolicy_vnode_setid_retain — OpenZFS calls with 3 args:
 *   (znode_t *zp, cred_t *cr, boolean_t issuidroot)
 * illumos native takes only 2 args:
 *   (const cred_t *, boolean_t)
 * Provide a wrapper macro that drops the znode argument.
 */
#define	secpolicy_vnode_setid_retain(zp, cr, flag)	\
	secpolicy_vnode_setid_retain(cr, flag)

#endif	/* _SPL_SYS_POLICY_H */
