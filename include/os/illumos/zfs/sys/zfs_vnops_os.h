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
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 * Copyright 2016 Nexenta Systems, Inc. All rights reserved.
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef	_SYS_FS_ZFS_VNOPS_OS_H
#define	_SYS_FS_ZFS_VNOPS_OS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * illumos ZFS vnode operations – OS-specific declarations.
 *
 * Core ZPL vnode operations (zfs_read, zfs_write, zfs_lookup, …) are
 * declared in the platform-neutral zfs_vnops.h.  This header adds the
 * illumos-specific variants that take illumos-native argument types.
 */

extern int zfs_remove(znode_t *dzp, const char *name, cred_t *cr, int flags);
extern int zfs_mkdir(znode_t *dzp, const char *dirname, vattr_t *vap,
    znode_t **zpp, cred_t *cr, int flags, vsecattr_t *vsecp,
    zidmap_t *mnt_ns);
extern int zfs_rmdir(znode_t *dzp, const char *name, znode_t *cwd,
    cred_t *cr, int flags);
extern int zfs_setattr(znode_t *zp, vattr_t *vap, int flag, cred_t *cr,
    zidmap_t *mnt_ns);
extern int zfs_rename(znode_t *sdzp, const char *snm, znode_t *tdzp,
    const char *tnm, cred_t *cr, int flags, uint64_t rflags, vattr_t *wo_vap,
    zidmap_t *mnt_ns);
extern int zfs_symlink(znode_t *dzp, const char *name, vattr_t *vap,
    const char *link, znode_t **zpp, cred_t *cr, int flags,
    zidmap_t *mnt_ns);
extern int zfs_link(znode_t *tdzp, znode_t *sp, const char *name,
    cred_t *cr, int flags);
extern int zfs_space(znode_t *zp, int cmd, flock64_t *bfp, int flag,
    offset_t offset, cred_t *cr);
extern int zfs_create(znode_t *dzp, const char *name, vattr_t *vap, int excl,
    int mode, znode_t **zpp, cred_t *cr, int flag, vsecattr_t *vsecp,
    zidmap_t *mnt_ns);
extern int zfs_setsecattr(znode_t *zp, vsecattr_t *vsecp, int flag,
    cred_t *cr);
extern int zfs_write_simple(znode_t *zp, const void *data, size_t len,
    loff_t pos, size_t *resid);

/*
 * Page I/O helpers used by the illumos segvn/page layer.
 */
extern caddr_t zfs_map_page(page_t *, enum seg_rw);
extern void zfs_unmap_page(page_t *, caddr_t);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_FS_ZFS_VNOPS_OS_H */
