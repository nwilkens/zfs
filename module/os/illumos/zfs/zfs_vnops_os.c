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
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ZFS vnode operations for illumos.
 * Stubs for now -- the full vnode operation implementations
 * (open, close, read, write, getattr, setattr, lookup, create,
 * remove, link, rename, mkdir, rmdir, readdir, fsync, seek,
 * space, etc.) will be ported from the illumos-joyent zfs_vnops.c.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/uio.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/zfs_context.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_ioctl.h>
#include <sys/fs/zfs.h>
#include <sys/dmu.h>
#include <sys/dmu_objset.h>
#include <sys/spa.h>
#include <sys/txg.h>
#include <sys/zap.h>
#include <sys/sa.h>
#include <sys/policy.h>
#include <sys/sunddi.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_quota.h>
#include <sys/zfs_sa.h>
#include <sys/zfs_rlock.h>
#include <sys/zfs_vnops.h>
#include <sys/zfs_znode.h>

/*
 * TODO: Implement zfs_open, zfs_close, zfs_read, zfs_write,
 * zfs_lookup, zfs_create, zfs_remove, zfs_mkdir, zfs_rmdir,
 * zfs_readdir, zfs_getattr, zfs_setattr, zfs_rename, zfs_symlink,
 * zfs_readlink, zfs_link, zfs_fsync, zfs_space, zfs_seek, etc.
 * These will be ported from the illumos-joyent zfs_vnops.c.
 */
