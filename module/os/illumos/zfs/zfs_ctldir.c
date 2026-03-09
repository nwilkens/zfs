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
 * ZFS control directory (.zfs) for illumos.
 * Stubs for now -- the .zfs/snapshot directory implementation
 * will be ported from the illumos-joyent zfs_ctldir.c using
 * the GFS (Generic Filesystem) primitives.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/kmem.h>
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_vfsops.h>

/*
 * TODO: Implement zfsctl_create, zfsctl_destroy,
 * zfsctl_root, zfsctl_lookup, zfsctl_snapdir_lookup,
 * zfsctl_snapshot_mount, zfsctl_snapshot_unmount, etc.
 * These will be ported from the illumos-joyent zfs_ctldir.c.
 */
