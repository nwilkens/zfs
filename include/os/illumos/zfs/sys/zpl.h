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
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef _SYS_ZPL_H
#define	_SYS_ZPL_H

/*
 * zpl.h is a Linux ZPL-specific header that exposes the Linux VFS
 * inode_operations / file_operations tables for ZFS.  It has no
 * equivalent on illumos, where the VFS interface is vnode-based and
 * the operation tables are registered via vfs_setfsops(9F) / vnodeops.
 *
 * This stub exists solely so that platform-neutral code that includes
 * <sys/zpl.h> compiles cleanly on illumos without modification.
 */

#endif	/* _SYS_ZPL_H */
