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
 * Copyright 2019 Joyent, Inc.
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef	_SYS_FS_ZFS_VFSOPS_OS_H
#define	_SYS_FS_ZFS_VFSOPS_OS_H

#include <sys/isa_defs.h>
#include <sys/types32.h>
#include <sys/list.h>
#include <sys/vfs.h>
#include <sys/zil.h>
#include <sys/sa.h>
#include <sys/rrwlock.h>
#include <sys/zfs_ioctl.h>
#include <sys/dsl_dataset.h>
#include <sys/dataset_kstats.h>
#include <sys/zone.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * illumos teardown lock types.
 *
 * z_teardown_lock uses the re-entrant readers/writers mutex (rrmlock_t) so
 * that ZFS_ENTER/ZFS_EXIT can be called recursively from within the same
 * thread (e.g. during replay).
 *
 * z_teardown_inactive_lock is a plain krwlock_t: inactive processing is
 * not re-entered.
 */
typedef rrmlock_t	zfs_teardown_lock_t;
typedef krwlock_t	zfs_teardown_inactive_lock_t;

typedef struct zfsvfs zfsvfs_t;
struct znode;

struct zfsvfs {
	vfs_t		*z_vfs;		/* generic fs struct */
	zfsvfs_t	*z_parent;	/* parent fs */
	objset_t	*z_os;		/* objset reference */
	uint64_t	z_flags;	/* super_block flags */
	uint64_t	z_root;		/* id of root znode */
	uint64_t	z_unlinkedobj;	/* id of unlinked zapobj */
	uint64_t	z_max_blksz;	/* maximum block size for files */
	uint64_t	z_fuid_obj;	/* fuid table object number */
	uint64_t	z_fuid_size;	/* fuid table size */
	avl_tree_t	z_fuid_idx;	/* fuid tree keyed by index */
	avl_tree_t	z_fuid_domain;	/* fuid tree keyed by domain */
	krwlock_t	z_fuid_lock;	/* fuid lock */
	boolean_t	z_fuid_loaded;	/* fuid tables are loaded */
	boolean_t	z_fuid_dirty;	/* need to sync fuid table? */
	struct zfs_fuid_info	*z_fuid_replay; /* fuid info for replay */
	zilog_t		*z_log;		/* intent log pointer */
	uint_t		z_acl_mode;	/* acl chmod/mode behavior */
	uint_t		z_acl_inherit;	/* acl inheritance behavior */
	boolean_t	z_acl_implicit;	/* acl implicit owner rights */
	zfs_case_t	z_case;		/* case-sense */
	boolean_t	z_utf8;		/* utf8-only */
	int		z_norm;		/* normalization flags */
	boolean_t	z_atime;	/* enable atimes mount option */
	boolean_t	z_unmounted;	/* unmounted */
	zfs_teardown_lock_t z_teardown_lock;
	zfs_teardown_inactive_lock_t z_teardown_inactive_lock;
	list_t		z_all_znodes;	/* all znodes in the fs */
	kmutex_t	z_znodes_lock;	/* lock for z_all_znodes */
	vnode_t		*z_rootdir;	/* mount root directory pointer */
	vnode_t		*z_ctldir;	/* .zfs directory pointer */
	boolean_t	z_show_ctldir;	/* expose .zfs in the root dir */
	boolean_t	z_issnap;	/* true if this is a snapshot */
	boolean_t	z_vscan;	/* virus scan on/off */
	boolean_t	z_use_fuids;	/* version allows fuids */
	boolean_t	z_replay;	/* set during ZIL replay */
	boolean_t	z_use_sa;	/* version allow system attributes */
	boolean_t	z_xattr_sa;	/* allow xattrs stored as SA */
	boolean_t	z_longname;	/* dataset supports long names */
	uint8_t		z_xattr;	/* xattr type in use */
	boolean_t	z_draining;	/* true when drain is active */
	boolean_t	z_drain_cancel;	/* signal unlinked drain to stop */
	uint64_t	z_version;	/* ZPL version */
	uint64_t	z_shares_dir;	/* hidden shares dir */
	kmutex_t	z_lock;
	uint64_t	z_userquota_obj;
	uint64_t	z_groupquota_obj;
	uint64_t	z_userobjquota_obj;
	uint64_t	z_groupobjquota_obj;
	uint64_t	z_projectquota_obj;
	uint64_t	z_projectobjquota_obj;
	dataset_kstats_t	z_kstat;	/* fs kstats */
	uint64_t	z_defaultuserquota;
	uint64_t	z_defaultgroupquota;
	uint64_t	z_defaultprojectquota;
	uint64_t	z_defaultuserobjquota;
	uint64_t	z_defaultgroupobjquota;
	uint64_t	z_defaultprojectobjquota;
	uint64_t	z_replay_eof;	/* new end of file - replay only */
	sa_attr_type_t	*z_attr_table;	/* SA attr mapping->id */
#define	ZFS_OBJ_MTX_SZ	64
	kmutex_t	z_hold_mtx[ZFS_OBJ_MTX_SZ]; /* znode hold locks */
	taskqid_t	z_drain_task;	/* task id for the unlink drain task */
	/* Zone from which this filesystem was mounted */
	zone_t		*z_zone;
};

/*
 * Teardown lock macros – illumos uses rrmlock_t (re-entrant RW mutex).
 */
#define	ZFS_TEARDOWN_INIT(zfsvfs)		\
	rrm_init(&(zfsvfs)->z_teardown_lock, B_FALSE)

#define	ZFS_TEARDOWN_DESTROY(zfsvfs)		\
	rrm_destroy(&(zfsvfs)->z_teardown_lock)

#define	ZFS_TEARDOWN_ENTER_READ(zfsvfs, tag)	\
	rrm_enter_read(&(zfsvfs)->z_teardown_lock, (tag))

#define	ZFS_TEARDOWN_EXIT_READ(zfsvfs, tag)	\
	rrm_exit(&(zfsvfs)->z_teardown_lock, (tag))

#define	ZFS_TEARDOWN_ENTER_WRITE(zfsvfs, tag)	\
	rrm_enter(&(zfsvfs)->z_teardown_lock, RW_WRITER, (tag))

#define	ZFS_TEARDOWN_EXIT_WRITE(zfsvfs)		\
	rrm_exit(&(zfsvfs)->z_teardown_lock, FTAG)

#define	ZFS_TEARDOWN_EXIT(zfsvfs, tag)		\
	rrm_exit(&(zfsvfs)->z_teardown_lock, (tag))

#define	ZFS_TEARDOWN_READ_HELD(zfsvfs)		\
	RRM_READ_HELD(&(zfsvfs)->z_teardown_lock)

#define	ZFS_TEARDOWN_WRITE_HELD(zfsvfs)		\
	RRM_WRITE_HELD(&(zfsvfs)->z_teardown_lock)

#define	ZFS_TEARDOWN_HELD(zfsvfs)		\
	RRM_LOCK_HELD(&(zfsvfs)->z_teardown_lock)

/*
 * Teardown-inactive lock macros – plain krwlock_t on illumos.
 */
#define	ZFS_TEARDOWN_INACTIVE_INIT(zfsvfs)		\
	rw_init(&(zfsvfs)->z_teardown_inactive_lock, NULL, RW_DEFAULT, NULL)

#define	ZFS_TEARDOWN_INACTIVE_DESTROY(zfsvfs)		\
	rw_destroy(&(zfsvfs)->z_teardown_inactive_lock)

#define	ZFS_TEARDOWN_INACTIVE_TRY_ENTER_READ(zfsvfs)	\
	rw_tryenter(&(zfsvfs)->z_teardown_inactive_lock, RW_READER)

#define	ZFS_TEARDOWN_INACTIVE_ENTER_READ(zfsvfs)	\
	rw_enter(&(zfsvfs)->z_teardown_inactive_lock, RW_READER)

#define	ZFS_TEARDOWN_INACTIVE_EXIT_READ(zfsvfs)		\
	rw_exit(&(zfsvfs)->z_teardown_inactive_lock)

#define	ZFS_TEARDOWN_INACTIVE_ENTER_WRITE(zfsvfs)	\
	rw_enter(&(zfsvfs)->z_teardown_inactive_lock, RW_WRITER)

#define	ZFS_TEARDOWN_INACTIVE_EXIT_WRITE(zfsvfs)	\
	rw_exit(&(zfsvfs)->z_teardown_inactive_lock)

#define	ZFS_TEARDOWN_INACTIVE_WRITE_HELD(zfsvfs)	\
	RW_WRITE_HELD(&(zfsvfs)->z_teardown_inactive_lock)

/*
 * File ID structures.
 *
 * Normal filesystems have a 12-byte file ID (including the 2-byte length
 * field) for NFSv2/v3 compatibility.  Snapshot filesystems use 22 bytes.
 */
typedef struct zfid_short {
	uint16_t	zf_len;
	uint8_t		zf_object[6];	/* obj[i] = obj >> (8 * i) */
	uint8_t		zf_gen[4];	/* gen[i] = gen >> (8 * i) */
} zfid_short_t;

typedef struct zfid_long {
	zfid_short_t	z_fid;
	uint8_t		zf_setid[6];	/* obj[i] = obj >> (8 * i) */
	uint8_t		zf_setgen[4];	/* gen[i] = gen >> (8 * i) */
} zfid_long_t;

#define	SHORT_FID_LEN	(sizeof (zfid_short_t) - sizeof (uint16_t))
#define	LONG_FID_LEN	(sizeof (zfid_long_t) - sizeof (uint16_t))

#define	ZSB_XATTR	0x0001		/* Enable user xattrs */

/*
 * Legacy ZFS_ENTER / ZFS_EXIT macros.
 *
 * Some illumos-specific code (e.g. zfs_acl.c, zfs_ctldir.c) still uses
 * the old-style macros that do not take a tag parameter or return a
 * value.  Provide them as wrappers around the modern zfs_enter/zfs_exit
 * that use FTAG.
 *
 * ZFS_ENTER calls zfs_enter() and returns EIO on failure.
 * ZFS_EXIT calls zfs_exit() with FTAG.
 */
#define	ZFS_ENTER(zfsvfs) \
	do { \
		int __zerr = zfs_enter((zfsvfs), FTAG); \
		if (__zerr != 0) \
			return (__zerr); \
	} while (0)

#define	ZFS_EXIT(zfsvfs)	zfs_exit((zfsvfs), FTAG)

extern void zfs_init(void);
extern void zfs_fini(void);

extern int zfs_suspend_fs(zfsvfs_t *zfsvfs);
extern int zfs_resume_fs(zfsvfs_t *zfsvfs, struct dsl_dataset *ds);
extern int zfs_end_fs(zfsvfs_t *zfsvfs, struct dsl_dataset *ds);
extern int zfs_set_version(zfsvfs_t *zfsvfs, uint64_t newvers);
extern int zfsvfs_create(const char *name, boolean_t readonly, zfsvfs_t **zfvp);
extern int zfsvfs_create_impl(zfsvfs_t **zfvp, zfsvfs_t *zfsvfs, objset_t *os);
extern void zfsvfs_free(zfsvfs_t *zfsvfs);
extern int zfs_check_global_label(const char *dsname, const char *hexsl);
extern boolean_t zfs_is_readonly(zfsvfs_t *zfsvfs);
extern int zfs_get_temporary_prop(struct dsl_dataset *ds, zfs_prop_t zfs_prop,
    uint64_t *val, char *setpoint);
extern int zfs_busy(void);
extern int zfs_userspace_one(zfsvfs_t *zfsvfs, zfs_userquota_prop_t type,
    const char *domain, uint64_t rid, uint64_t *valuep);
extern int zfs_userspace_many(zfsvfs_t *zfsvfs, zfs_userquota_prop_t type,
    uint64_t *cookiep, void *vbuf, uint64_t *bufsizep, uint64_t *offp);
extern int zfs_set_userquota(zfsvfs_t *zfsvfs, zfs_userquota_prop_t type,
    const char *domain, uint64_t rid, uint64_t quota);
extern int zfs_set_default_quota(zfsvfs_t *zfsvfs, zfs_prop_t zfs_prop,
    uint64_t value);

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_FS_ZFS_VFSOPS_OS_H */
