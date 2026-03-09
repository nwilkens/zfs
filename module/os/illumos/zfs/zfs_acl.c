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
 * Copyright (c) 2013 by Delphix. All rights reserved.
 * Copyright 2017 Nexenta Systems, Inc. All rights reserved.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ZFS ACL support for illumos.
 * Stubs for now -- the full NFSv4 ACL implementation will be
 * ported from the illumos-joyent zfs_acl.c.
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
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/sdt.h>
#include <sys/fs/zfs.h>
#include <sys/policy.h>
#include <sys/zfs_context.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_acl.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_quota.h>
#include <sys/zfs_vfsops.h>
#include <sys/dmu.h>
#include <sys/dnode.h>
#include <sys/zap.h>
#include <sys/sa.h>

/*
 * TODO: Implement zfs_acl_chmod, zfs_acl_chown,
 * zfs_getacl, zfs_setacl, zfs_acl_access,
 * zfs_zaccess, zfs_zaccess_rwx, zfs_zaccess_unix,
 * zfs_acl_ids_create, zfs_acl_ids_free, etc.
 * These will be ported from the illumos-joyent zfs_acl.c.
 */
