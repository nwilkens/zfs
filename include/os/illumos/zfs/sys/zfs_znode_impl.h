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
 * Copyright (c) 2012, 2018 by Delphix. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 * Copyright 2016 Nexenta Systems, Inc. All rights reserved.
 * Copyright 2022-2023 Racktop Systems, Inc.
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef	_ILLUMOS_ZFS_SYS_ZNODE_IMPL_H
#define	_ILLUMOS_ZFS_SYS_ZNODE_IMPL_H

#include <sys/list.h>
#include <sys/dmu.h>
#include <sys/sa.h>
#include <sys/zfs_vfsops.h>
#include <sys/rrwlock.h>
#include <sys/zfs_sa.h>
#include <sys/zfs_stat.h>
#include <sys/zfs_rlock.h>
#include <sys/zfs_acl.h>
#include <sys/zil.h>
#include <sys/zfs_project.h>
#include <sys/vnode.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * OS-specific fields embedded in struct znode.
 *
 * On illumos, every znode is paired 1:1 with a vnode_t.  The z_zfsvfs
 * back-pointer and the cached uid/gid/gen/atime/links are also kept here
 * so that they can be accessed without touching the DMU buffer.
 */
#define	ZNODE_OS_FIELDS			\
	struct zfsvfs	*z_zfsvfs;	\
	vnode_t		*z_vnode;	\
	uint64_t	z_uid;		\
	uint64_t	z_gid;		\
	uint64_t	z_gen;		\
	uint64_t	z_atime[2];	\
	uint64_t	z_links;

#define	ZFS_LINK_MAX	UINT64_MAX

/*
 * ZFS minor numbers can refer to either a control device instance or
 * a zvol.  Depending on the value of zss_type, zss_data points to either
 * a zvol_state_t or a zfs_onexit_t.
 */
enum zfs_soft_state_type {
	ZSST_ZVOL,
	ZSST_CTLDEV
};

typedef struct zfs_soft_state {
	enum zfs_soft_state_type zss_type;
	void *zss_data;
} zfs_soft_state_t;

/*
 * Convert between znode pointers and vnode pointers.
 */
#define	ZTOV(ZP)	((ZP)->z_vnode)
#define	ZTOI(ZP)	((ZP)->z_vnode)
#define	VTOZ(VP)	((znode_t *)(VP)->v_data)
#define	ITOZ(VP)	((znode_t *)(VP)->v_data)
#define	zhold(zp)	VN_HOLD(ZTOV((zp)))
#define	zrele(zp)	VN_RELE(ZTOV((zp)))

#define	ZTOZSB(zp)	((zp)->z_zfsvfs)
#define	ITOZSB(vp)	(VTOZ(vp)->z_zfsvfs)
#define	ZTOTYPE(zp)	(ZTOV(zp)->v_type)
#define	ZTOGID(zp)	((zp)->z_gid)
#define	ZTOUID(zp)	((zp)->z_uid)
#define	ZTONLNK(zp)	((zp)->z_links)
#define	Z_ISBLK(type)	((type) == VBLK)
#define	Z_ISCHR(type)	((type) == VCHR)
#define	Z_ISLNK(type)	((type) == VLNK)
#define	Z_ISDIR(type)	((type) == VDIR)

#define	zn_has_cached_data(zp, start, end)	vn_has_cached_data(ZTOV(zp))
#define	zn_flush_cached_data(zp, sync)		vn_flush_cached_data(ZTOV(zp), \
						    (sync))
#define	zn_rlimit_fsize(size)			(0)
#define	zn_rlimit_fsize_uio(zp, uio)		(0)

/*
 * Called on entry to each ZFS vnode and vfs operation.
 * Uses the illumos rrmlock_t re-entrant teardown lock.
 */
static inline int
zfs_enter(zfsvfs_t *zfsvfs, const char *tag)
{
	ZFS_TEARDOWN_ENTER_READ(zfsvfs, tag);
	if (__builtin_expect((zfsvfs)->z_unmounted, 0)) {
		ZFS_TEARDOWN_EXIT_READ(zfsvfs, tag);
		return (SET_ERROR(EIO));
	}
	return (0);
}

/* Must be called before exiting the vop. */
static inline void
zfs_exit(zfsvfs_t *zfsvfs, const char *tag)
{
	ZFS_TEARDOWN_EXIT_READ(zfsvfs, tag);
}

/*
 * Macros for dealing with dmu_buf_hold.
 */
#define	ZFS_OBJ_HASH(obj_num)	((obj_num) & (ZFS_OBJ_MTX_SZ - 1))
#define	ZFS_OBJ_MUTEX(zfsvfs, obj_num)	\
	(&(zfsvfs)->z_hold_mtx[ZFS_OBJ_HASH(obj_num)])
#define	ZFS_OBJ_HOLD_ENTER(zfsvfs, obj_num) \
	mutex_enter(ZFS_OBJ_MUTEX((zfsvfs), (obj_num)))
#define	ZFS_OBJ_HOLD_TRYENTER(zfsvfs, obj_num) \
	mutex_tryenter(ZFS_OBJ_MUTEX((zfsvfs), (obj_num)))
#define	ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num) \
	mutex_exit(ZFS_OBJ_MUTEX((zfsvfs), (obj_num)))

/* Encode ZFS stored time values from a struct timespec. */
#define	ZFS_TIME_ENCODE(tp, stmp)		\
{						\
	(stmp)[0] = (uint64_t)(tp)->tv_sec;	\
	(stmp)[1] = (uint64_t)(tp)->tv_nsec;	\
}

/* Decode ZFS stored time values to a struct timespec. */
#define	ZFS_TIME_DECODE(tp, stmp)		\
{						\
	(tp)->tv_sec = (time_t)(stmp)[0];	\
	(tp)->tv_nsec = (long)(stmp)[1];	\
}

#define	ZFS_ACCESSTIME_STAMP(zfsvfs, zp) \
	if ((zfsvfs)->z_atime && !((zfsvfs)->z_vfs->vfs_flag & VFS_RDONLY)) \
		zfs_tstamp_update_setup(zp, ACCESSED, NULL, NULL);

extern void zfs_tstamp_update_setup(struct znode *, uint_t, uint64_t [2],
    uint64_t [2]);
extern void zfs_znode_free(struct znode *);

extern zil_replay_func_t *const zfs_replay_vector[TX_MAX_TYPE];

extern int zfs_znode_parent_and_name(struct znode *zp, struct znode **dzpp,
    char *buf, uint64_t buflen);

#ifdef	__cplusplus
}
#endif

#endif	/* _ILLUMOS_ZFS_SYS_ZNODE_IMPL_H */
