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
 * Copyright (c) 2013, 2016 by Delphix. All rights reserved.
 * Copyright 2017 Nexenta Systems, Inc.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ZFS directory operations for illumos.
 * Stubs for now -- the full directory operation implementations
 * will be ported from the illumos-joyent zfs_dir.c.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/kmem.h>
#include <sys/uio.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/zfs_context.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_acl.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_znode.h>
#include <sys/dmu.h>
#include <sys/dmu_objset.h>
#include <sys/zap.h>
#include <sys/dmu_tx.h>

/*
 * TODO: Implement zfs_dirent_lock, zfs_dirent_unlock,
 * zfs_dirlook, zfs_link_create, zfs_link_destroy,
 * zfs_make_xattrdir, zfs_unlinked_add, zfs_unlinked_drain, etc.
 * These will be ported from the illumos-joyent zfs_dir.c.
 */
