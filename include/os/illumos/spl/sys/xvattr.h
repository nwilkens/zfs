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

#ifndef _SPL_SYS_XVATTR_H
#define	_SPL_SYS_XVATTR_H

/*
 * illumos defines xoptattr_t, xvattr_t, vattr_t, and the XVA_*
 * macros natively in <sys/vnode.h>.  OpenZFS has its own
 * include/sys/xvattr.h that redefines these for Linux/FreeBSD
 * where they don't exist.
 *
 * On illumos we must prevent that redefinition.  We do this by
 * defining _SYS_XVATTR_H (the OpenZFS guard) before anyone can
 * include the OpenZFS version, and providing the few extras that
 * the OpenZFS xvattr.h adds beyond the illumos native definitions.
 */
#define	_SYS_XVATTR_H

#include <sys/vnode.h>
#include <sys/string.h>

/*
 * AV_SCANSTAMP_SZ — anti-virus scanstamp length.
 * Already defined in illumos <sys/vnode.h>.
 */
#ifndef AV_SCANSTAMP_SZ
#define	AV_SCANSTAMP_SZ	32
#endif

/*
 * ATTR_XVATTR — OpenZFS name for AT_XVATTR (illumos native).
 * Used in xva_init() and a few neutral ZFS files.
 */
#ifndef ATTR_XVATTR
#define	ATTR_XVATTR	AT_XVATTR
#endif

/*
 * xoptattr_t, xvattr_t, vattr_t, XVA_SET_REQ, XVA_CLR_REQ,
 * XVA_SET_RTN, XVA_ISSET_REQ, XVA_ISSET_RTN, xva_init(), xoap_get()
 * — all native on illumos via <sys/vnode.h>.
 */

#endif	/* _SPL_SYS_XVATTR_H */
