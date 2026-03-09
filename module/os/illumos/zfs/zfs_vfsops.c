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
 * Copyright 2020 Joyent, Inc.
 * Copyright (c) 2025, OpenZFS on illumos.
 */

/* Portions Copyright 2010 Robert Milkowski */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/pathname.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/vfs_opreg.h>
#include <sys/mntent.h>
#include <sys/mount.h>
#include <sys/cmn_err.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_vnops.h>
#include <sys/zfs_dir.h>
#include <sys/zil.h>
#include <sys/fs/zfs.h>
#include <sys/dmu.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_deleg.h>
#include <sys/spa.h>
#include <sys/zap.h>
#include <sys/sa.h>
#include <sys/sa_impl.h>
#include <sys/policy.h>
#include <sys/atomic.h>
#include <sys/mkdev.h>
#include <sys/modctl.h>
#include <sys/refstr.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_sa.h>
#include <sys/sunddi.h>
#include <sys/dnlc.h>
#include <sys/dmu_objset.h>
#include <sys/dsl_dir.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_quota.h>
#include <sys/dataset_kstats.h>

#include "zfs_comutil.h"
#include <sys/txg.h>

/*
 * Check if a dataset has outstanding dirty data in any txg.
 * This is used during unmount to decide whether to wait for sync.
 */
static boolean_t
dsl_dataset_is_dirty(dsl_dataset_t *ds)
{
	for (int t = 0; t < TXG_SIZE; t++) {
		if (txg_list_member(&ds->ds_dir->dd_pool->dp_dirty_datasets,
		    ds, t))
			return (B_TRUE);
	}
	return (B_FALSE);
}

/*
 * Minimum minor number for ZFS device nodes.  On illumos the ZFS driver
 * reserves minor 0 for the control device, so filesystem instances start
 * at minor 1.
 */
#ifndef ZFS_MIN_MINOR
#define	ZFS_MIN_MINOR	1
#endif

int zfsfstype;
vfsops_t *zfs_vfsops = NULL;
static major_t zfs_major;
static minor_t zfs_minor;
static kmutex_t	zfs_dev_mtx;

extern int sys_shutdown;

static int zfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr);
static int zfs_umount(vfs_t *vfsp, int fflag, cred_t *cr);
static int zfs_root(vfs_t *vfsp, vnode_t **vpp);
static int zfs_statvfs(vfs_t *vfsp, struct statvfs64 *statp);
static int zfs_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp);
static void zfs_freevfs(vfs_t *vfsp);

int zfs_sync(vfs_t *vfsp, short flag, cred_t *cr);

static const fs_operation_def_t zfs_vfsops_template[] = {
	VFSNAME_MOUNT,		{ .vfs_mount = zfs_mount },
	VFSNAME_UNMOUNT,	{ .vfs_unmount = zfs_umount },
	VFSNAME_ROOT,		{ .vfs_root = zfs_root },
	VFSNAME_STATVFS,	{ .vfs_statvfs = zfs_statvfs },
	VFSNAME_SYNC,		{ .vfs_sync = (fs_generic_func_p)zfs_sync },
	VFSNAME_VGET,		{ .vfs_vget = zfs_vget },
	VFSNAME_FREEVFS,	{ .vfs_freevfs = zfs_freevfs },
	NULL,			NULL
};

/*
 * We need to keep a count of active fs's.
 * This is necessary to prevent our module
 * from being unloaded after a umount -f
 */
static uint32_t	zfs_active_fs_count = 0;

static char *noatime_cancel[] = { MNTOPT_ATIME, NULL };
static char *atime_cancel[] = { MNTOPT_NOATIME, NULL };
static char *noxattr_cancel[] = { MNTOPT_XATTR, NULL };
static char *xattr_cancel[] = { MNTOPT_NOXATTR, NULL };

/*
 * MO_DEFAULT is not used since the default value is determined
 * by the equivalent property.
 */
static mntopt_t mntopts[] = {
	{ MNTOPT_NOXATTR, noxattr_cancel, NULL, 0, NULL },
	{ MNTOPT_XATTR, xattr_cancel, NULL, 0, NULL },
	{ MNTOPT_NOATIME, noatime_cancel, NULL, 0, NULL },
	{ MNTOPT_ATIME, atime_cancel, NULL, 0, NULL }
};

static mntopts_t zfs_mntopts = {
	sizeof (mntopts) / sizeof (mntopt_t),
	mntopts
};

/*ARGSUSED*/
int
zfs_sync(vfs_t *vfsp, short flag, cred_t *cr)
{
	/*
	 * Data integrity is job one.  We don't want a compromised kernel
	 * writing to the storage pool, so we never sync during panic.
	 */
	if (panicstr)
		return (0);

	/*
	 * SYNC_ATTR is used by fsflush() to force old filesystems like UFS
	 * to sync metadata, which they would otherwise cache indefinitely.
	 * Semantically, the only requirement is that the sync be initiated.
	 * The DMU syncs out txgs frequently, so there's nothing to do.
	 */
	if (flag & SYNC_ATTR)
		return (0);

	if (vfsp != NULL) {
		/*
		 * Sync a specific filesystem.
		 */
		zfsvfs_t *zfsvfs = vfsp->vfs_data;
		dsl_pool_t *dp;
		int error;

		if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
			return (error);
		dp = dmu_objset_pool(zfsvfs->z_os);

		/*
		 * If the system is shutting down, then skip any
		 * filesystems which may exist on a suspended pool.
		 */
		if (sys_shutdown && spa_suspended(dp->dp_spa)) {
			zfs_exit(zfsvfs, FTAG);
			return (0);
		}

		if (zfsvfs->z_log != NULL)
			zil_commit(zfsvfs->z_log, 0);

		zfs_exit(zfsvfs, FTAG);
	} else {
		/*
		 * Sync all ZFS filesystems.  This is what happens when you
		 * run sync(8).  Unlike other filesystems, ZFS honors the
		 * request by waiting for all pools to commit all dirty data.
		 */
		spa_sync_allpools();
	}

	return (0);
}

static int
zfs_create_unique_device(dev_t *dev)
{
	major_t new_major;

	do {
		ASSERT3U(zfs_minor, <=, MAXMIN32);
		minor_t start = zfs_minor;
		do {
			mutex_enter(&zfs_dev_mtx);
			if (zfs_minor >= MAXMIN32) {
				if (zfs_major == ddi_name_to_major(ZFS_DRIVER))
					zfs_minor = ZFS_MIN_MINOR;
				else
					zfs_minor = 0;
			} else {
				zfs_minor++;
			}
			*dev = makedevice(zfs_major, zfs_minor);
			mutex_exit(&zfs_dev_mtx);
		} while (vfs_devismounted(*dev) && zfs_minor != start);
		if (zfs_minor == start) {
			if ((new_major = getudev()) == (major_t)-1) {
				cmn_err(CE_WARN,
				    "zfs_mount: Can't get unique major "
				    "device number.");
				return (-1);
			}
			mutex_enter(&zfs_dev_mtx);
			zfs_major = new_major;
			zfs_minor = 0;
			mutex_exit(&zfs_dev_mtx);
		} else {
			break;
		}
		/* CONSTANTCONDITION */
	} while (1);

	return (0);
}

static void
atime_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	if (newval == TRUE) {
		zfsvfs->z_atime = TRUE;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NOATIME);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_ATIME, NULL, 0);
	} else {
		zfsvfs->z_atime = FALSE;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_ATIME);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NOATIME, NULL, 0);
	}
}

static void
xattr_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	if (newval == TRUE) {
		/* XXX locking on vfs_flag? */
		zfsvfs->z_vfs->vfs_flag |= VFS_XATTR;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NOXATTR);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_XATTR, NULL, 0);
	} else {
		/* XXX locking on vfs_flag? */
		zfsvfs->z_vfs->vfs_flag &= ~VFS_XATTR;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_XATTR);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NOXATTR, NULL, 0);
	}
}

static void
blksz_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;
	ASSERT3U(newval, <=, spa_maxblocksize(dmu_objset_spa(zfsvfs->z_os)));
	ASSERT3U(newval, >=, SPA_MINBLOCKSIZE);
	ASSERT(ISP2(newval));

	zfsvfs->z_max_blksz = newval;
	zfsvfs->z_vfs->vfs_bsize = newval;
}

static void
readonly_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	if (newval) {
		/* XXX locking on vfs_flag? */
		zfsvfs->z_vfs->vfs_flag |= VFS_RDONLY;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_RW);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_RO, NULL, 0);
	} else {
		/* XXX locking on vfs_flag? */
		zfsvfs->z_vfs->vfs_flag &= ~VFS_RDONLY;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_RO);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_RW, NULL, 0);
	}
}

static void
devices_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	if (newval == FALSE) {
		zfsvfs->z_vfs->vfs_flag |= VFS_NODEVICES;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_DEVICES);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NODEVICES, NULL, 0);
	} else {
		zfsvfs->z_vfs->vfs_flag &= ~VFS_NODEVICES;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NODEVICES);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_DEVICES, NULL, 0);
	}
}

static void
setuid_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	if (newval == FALSE) {
		zfsvfs->z_vfs->vfs_flag |= VFS_NOSETUID;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_SETUID);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NOSETUID, NULL, 0);
	} else {
		zfsvfs->z_vfs->vfs_flag &= ~VFS_NOSETUID;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NOSETUID);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_SETUID, NULL, 0);
	}
}

static void
exec_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	if (newval == FALSE) {
		zfsvfs->z_vfs->vfs_flag |= VFS_NOEXEC;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_EXEC);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NOEXEC, NULL, 0);
	} else {
		zfsvfs->z_vfs->vfs_flag &= ~VFS_NOEXEC;
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NOEXEC);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_EXEC, NULL, 0);
	}
}

/*
 * The nbmand mount option can be changed at mount time.
 * We can't allow it to be toggled on live file systems or incorrect
 * behavior may be seen from cifs clients
 *
 * This property isn't registered via dsl_prop_register(), but this callback
 * will be called when a file system is first mounted
 */
static void
nbmand_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;
	if (newval == FALSE) {
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NBMAND);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NONBMAND, NULL, 0);
	} else {
		vfs_clearmntopt(zfsvfs->z_vfs, MNTOPT_NONBMAND);
		vfs_setmntopt(zfsvfs->z_vfs, MNTOPT_NBMAND, NULL, 0);
	}
}

static void
snapdir_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	zfsvfs->z_show_ctldir = newval;
}

static void
vscan_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	zfsvfs->z_vscan = newval;
}

static void
acl_mode_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	zfsvfs->z_acl_mode = newval;
}

static void
acl_inherit_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	zfsvfs->z_acl_inherit = newval;
}

static void
acl_implicit_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	zfsvfs->z_acl_implicit = (boolean_t)newval;
}

static void
longname_changed_cb(void *arg, uint64_t newval)
{
	zfsvfs_t *zfsvfs = arg;

	zfsvfs->z_longname = newval;
}

static int
zfs_register_callbacks(vfs_t *vfsp)
{
	struct dsl_dataset *ds = NULL;
	objset_t *os = NULL;
	zfsvfs_t *zfsvfs = NULL;
	uint64_t nbmand;
	boolean_t readonly = B_FALSE;
	boolean_t do_readonly = B_FALSE;
	boolean_t setuid = B_FALSE;
	boolean_t do_setuid = B_FALSE;
	boolean_t exec = B_FALSE;
	boolean_t do_exec = B_FALSE;
	boolean_t devices = B_FALSE;
	boolean_t do_devices = B_FALSE;
	boolean_t xattr = B_FALSE;
	boolean_t do_xattr = B_FALSE;
	boolean_t atime = B_FALSE;
	boolean_t do_atime = B_FALSE;
	int error = 0;

	ASSERT(vfsp);
	zfsvfs = vfsp->vfs_data;
	ASSERT(zfsvfs);
	os = zfsvfs->z_os;

	/*
	 * This function can be called for a snapshot when we update snapshot's
	 * mount point, which isn't really supported.
	 */
	if (dmu_objset_is_snapshot(os))
		return (EOPNOTSUPP);

	/*
	 * The act of registering our callbacks will destroy any mount
	 * options we may have.  In order to enable temporary overrides
	 * of mount options, we stash away the current values and
	 * restore them after we register the callbacks.
	 */
	if (vfs_optionisset(vfsp, MNTOPT_RO, NULL) ||
	    !spa_writeable(dmu_objset_spa(os))) {
		readonly = B_TRUE;
		do_readonly = B_TRUE;
	} else if (vfs_optionisset(vfsp, MNTOPT_RW, NULL)) {
		readonly = B_FALSE;
		do_readonly = B_TRUE;
	}
	if (vfs_optionisset(vfsp, MNTOPT_NOSETUID, NULL)) {
		devices = B_FALSE;
		setuid = B_FALSE;
		do_devices = B_TRUE;
		do_setuid = B_TRUE;
	} else {
		if (vfs_optionisset(vfsp, MNTOPT_NODEVICES, NULL)) {
			devices = B_FALSE;
			do_devices = B_TRUE;
		} else if (vfs_optionisset(vfsp, MNTOPT_DEVICES, NULL)) {
			devices = B_TRUE;
			do_devices = B_TRUE;
		}

		if (vfs_optionisset(vfsp, MNTOPT_NOSETUID, NULL)) {
			setuid = B_FALSE;
			do_setuid = B_TRUE;
		} else if (vfs_optionisset(vfsp, MNTOPT_SETUID, NULL)) {
			setuid = B_TRUE;
			do_setuid = B_TRUE;
		}
	}
	if (vfs_optionisset(vfsp, MNTOPT_NOEXEC, NULL)) {
		exec = B_FALSE;
		do_exec = B_TRUE;
	} else if (vfs_optionisset(vfsp, MNTOPT_EXEC, NULL)) {
		exec = B_TRUE;
		do_exec = B_TRUE;
	}
	if (vfs_optionisset(vfsp, MNTOPT_NOXATTR, NULL)) {
		xattr = B_FALSE;
		do_xattr = B_TRUE;
	} else if (vfs_optionisset(vfsp, MNTOPT_XATTR, NULL)) {
		xattr = B_TRUE;
		do_xattr = B_TRUE;
	}
	if (vfs_optionisset(vfsp, MNTOPT_NOATIME, NULL)) {
		atime = B_FALSE;
		do_atime = B_TRUE;
	} else if (vfs_optionisset(vfsp, MNTOPT_ATIME, NULL)) {
		atime = B_TRUE;
		do_atime = B_TRUE;
	}

	/*
	 * nbmand is a special property.  It can only be changed at
	 * mount time.
	 *
	 * This is weird, but it is documented to only be changeable
	 * at mount time.
	 */
	if (vfs_optionisset(vfsp, MNTOPT_NONBMAND, NULL)) {
		nbmand = B_FALSE;
	} else if (vfs_optionisset(vfsp, MNTOPT_NBMAND, NULL)) {
		nbmand = B_TRUE;
	} else {
		char osname[ZFS_MAX_DATASET_NAME_LEN];

		dmu_objset_name(os, osname);
		if ((error = dsl_prop_get_integer(osname, "nbmand", &nbmand,
		    NULL)) != 0) {
			return (error);
		}
	}

	/*
	 * Register property callbacks.
	 *
	 * It would probably be fine to just check for i/o error from
	 * the first prop_register(), but I guess I like to go
	 * overboard...
	 */
	ds = dmu_objset_ds(os);
	dsl_pool_config_enter(dmu_objset_pool(os), FTAG);
	error = dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_ATIME), atime_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_XATTR), xattr_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_RECORDSIZE), blksz_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_READONLY), readonly_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_DEVICES), devices_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_SETUID), setuid_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_EXEC), exec_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_SNAPDIR), snapdir_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_ACLMODE), acl_mode_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_ACLINHERIT), acl_inherit_changed_cb,
	    zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_VSCAN), vscan_changed_cb, zfsvfs);
	error = error ? error : dsl_prop_register(ds,
	    zfs_prop_to_name(ZFS_PROP_LONGNAME), longname_changed_cb, zfsvfs);
	dsl_pool_config_exit(dmu_objset_pool(os), FTAG);
	if (error)
		goto unregister;

	/*
	 * Invoke our callbacks to restore temporary mount options.
	 */
	if (do_readonly)
		readonly_changed_cb(zfsvfs, readonly);
	if (do_setuid)
		setuid_changed_cb(zfsvfs, setuid);
	if (do_exec)
		exec_changed_cb(zfsvfs, exec);
	if (do_devices)
		devices_changed_cb(zfsvfs, devices);
	if (do_xattr)
		xattr_changed_cb(zfsvfs, xattr);
	if (do_atime)
		atime_changed_cb(zfsvfs, atime);

	nbmand_changed_cb(zfsvfs, nbmand);

	return (0);

unregister:
	dsl_prop_unregister_all(ds, zfsvfs);
	return (error);
}

static int
zfs_space_delta_cb(dmu_object_type_t bonustype, void *data,
    uint64_t *userp, uint64_t *groupp, uint64_t *projectp)
{
	sa_hdr_phys_t sa;
	sa_hdr_phys_t *sap = data;
	uint64_t flags;
	int hdrsize;
	boolean_t swap = B_FALSE;

	/*
	 * Is it a valid type of object to track?
	 */
	if (bonustype != DMU_OT_ZNODE && bonustype != DMU_OT_SA)
		return (SET_ERROR(ENOENT));

	/*
	 * If we have a NULL data pointer
	 * then assume the id's aren't changing and
	 * return EEXIST to the dmu to let it know to
	 * use the same ids
	 */
	if (data == NULL)
		return (SET_ERROR(EEXIST));

	if (bonustype == DMU_OT_ZNODE) {
		znode_phys_t *znp = data;
		*userp = znp->zp_uid;
		*groupp = znp->zp_gid;
		*projectp = ZFS_DEFAULT_PROJID;
		return (0);
	}

	if (sap->sa_magic == 0) {
		/*
		 * This should only happen for newly created files
		 * that haven't had the znode data filled in yet.
		 */
		*userp = 0;
		*groupp = 0;
		*projectp = ZFS_DEFAULT_PROJID;
		return (0);
	}

	sa = *sap;
	if (sa.sa_magic == BSWAP_32(SA_MAGIC)) {
		sa.sa_magic = SA_MAGIC;
		sa.sa_layout_info = BSWAP_16(sa.sa_layout_info);
		swap = B_TRUE;
	} else {
		VERIFY3U(sa.sa_magic, ==, SA_MAGIC);
	}

	hdrsize = sa_hdrsize(&sa);
	VERIFY3U(hdrsize, >=, sizeof (sa_hdr_phys_t));

	*userp = *((uint64_t *)((uintptr_t)data + hdrsize + SA_UID_OFFSET));
	*groupp = *((uint64_t *)((uintptr_t)data + hdrsize + SA_GID_OFFSET));
	flags = *((uint64_t *)((uintptr_t)data + hdrsize + SA_FLAGS_OFFSET));
	if (swap)
		flags = BSWAP_64(flags);

	if (flags & ZFS_PROJID)
		*projectp = *((uint64_t *)((uintptr_t)data + hdrsize +
		    SA_PROJID_OFFSET));
	else
		*projectp = ZFS_DEFAULT_PROJID;

	if (swap) {
		*userp = BSWAP_64(*userp);
		*groupp = BSWAP_64(*groupp);
		*projectp = BSWAP_64(*projectp);
	}
	return (0);
}

static void
fuidstr_to_sid(zfsvfs_t *zfsvfs, const char *fuidstr,
    char *domainbuf, int buflen, uid_t *ridp)
{
	uint64_t fuid;
	const char *domain;

	fuid = zfs_strtonum(fuidstr, NULL);

	domain = zfs_fuid_find_by_idx(zfsvfs, FUID_INDEX(fuid));
	if (domain)
		(void) strlcpy(domainbuf, domain, buflen);
	else
		domainbuf[0] = '\0';
	*ridp = FUID_RID(fuid);
}

static uint64_t
zfs_userquota_prop_to_obj(zfsvfs_t *zfsvfs, zfs_userquota_prop_t type)
{
	switch (type) {
	case ZFS_PROP_USERUSED:
	case ZFS_PROP_USEROBJUSED:
		return (DMU_USERUSED_OBJECT);
	case ZFS_PROP_GROUPUSED:
	case ZFS_PROP_GROUPOBJUSED:
		return (DMU_GROUPUSED_OBJECT);
	case ZFS_PROP_PROJECTUSED:
	case ZFS_PROP_PROJECTOBJUSED:
		return (DMU_PROJECTUSED_OBJECT);
	case ZFS_PROP_USERQUOTA:
		return (zfsvfs->z_userquota_obj);
	case ZFS_PROP_GROUPQUOTA:
		return (zfsvfs->z_groupquota_obj);
	case ZFS_PROP_USEROBJQUOTA:
		return (zfsvfs->z_userobjquota_obj);
	case ZFS_PROP_GROUPOBJQUOTA:
		return (zfsvfs->z_groupobjquota_obj);
	case ZFS_PROP_PROJECTQUOTA:
		return (zfsvfs->z_projectquota_obj);
	case ZFS_PROP_PROJECTOBJQUOTA:
		return (zfsvfs->z_projectobjquota_obj);
	default:
		return (ZFS_NO_OBJECT);
	}
}

static int
id_to_fuidstr(zfsvfs_t *zfsvfs, const char *domain, uid_t rid,
    char *buf, int buflen, boolean_t addok)
{
	uint64_t fuid;
	int domainid = 0;

	if (domain && domain[0]) {
		domainid = zfs_fuid_find_by_domain(zfsvfs, domain, NULL, addok);
		if (domainid == -1)
			return (SET_ERROR(ENOENT));
	}
	fuid = FUID_ENCODE(domainid, rid);
	(void) snprintf(buf, buflen, "%llx", (longlong_t)fuid);
	return (0);
}

/*
 * Associate this zfsvfs with the given objset, which must be owned.
 * This will cache a bunch of on-disk state from the objset in the
 * zfsvfs.
 */
static int
zfsvfs_init(zfsvfs_t *zfsvfs, objset_t *os)
{
	int error;
	uint64_t val;

	zfsvfs->z_max_blksz = SPA_OLD_MAXBLOCKSIZE;
	zfsvfs->z_show_ctldir = ZFS_SNAPDIR_VISIBLE;
	zfsvfs->z_os = os;

	error = zfs_get_zplprop(os, ZFS_PROP_VERSION, &zfsvfs->z_version);
	if (error != 0)
		return (error);
	if (zfsvfs->z_version >
	    zfs_zpl_version_map(spa_version(dmu_objset_spa(os)))) {
		(void) printf("Can't mount a version %lld file system "
		    "on a version %lld pool\n. Pool must be upgraded to mount "
		    "this file system.", (u_longlong_t)zfsvfs->z_version,
		    (u_longlong_t)spa_version(dmu_objset_spa(os)));
		return (SET_ERROR(ENOTSUP));
	}
	error = zfs_get_zplprop(os, ZFS_PROP_NORMALIZE, &val);
	if (error != 0)
		return (error);
	zfsvfs->z_norm = (int)val;

	error = zfs_get_zplprop(os, ZFS_PROP_UTF8ONLY, &val);
	if (error != 0)
		return (error);
	zfsvfs->z_utf8 = (val != 0);

	error = zfs_get_zplprop(os, ZFS_PROP_CASE, &val);
	if (error != 0)
		return (error);
	zfsvfs->z_case = (uint_t)val;

	error = zfs_get_zplprop(os, ZFS_PROP_ACLTYPE, &val);
	if (error != 0)
		return (error);

	/*
	 * Fold case on file systems that are always or sometimes case
	 * insensitive.
	 */
	if (zfsvfs->z_case == ZFS_CASE_INSENSITIVE ||
	    zfsvfs->z_case == ZFS_CASE_MIXED)
		zfsvfs->z_norm |= U8_TEXTPREP_TOUPPER;

	zfsvfs->z_use_fuids = USE_FUIDS(zfsvfs->z_version, zfsvfs->z_os);
	zfsvfs->z_use_sa = USE_SA(zfsvfs->z_version, zfsvfs->z_os);

	uint64_t sa_obj = 0;
	if (zfsvfs->z_use_sa) {
		/* should either have both of these objects or none */
		error = zap_lookup(os, MASTER_NODE_OBJ, ZFS_SA_ATTRS, 8, 1,
		    &sa_obj);
		if (error != 0)
			return (error);

		error = zfs_get_zplprop(os, ZFS_PROP_XATTR, &val);
		if (error == 0 && val == ZFS_XATTR_SA)
			zfsvfs->z_xattr_sa = B_TRUE;
	}

	error = zfs_get_zplprop(os, ZFS_PROP_DEFAULTUSERQUOTA,
	    &zfsvfs->z_defaultuserquota);
	if (error != 0)
		return (error);

	error = zfs_get_zplprop(os, ZFS_PROP_DEFAULTGROUPQUOTA,
	    &zfsvfs->z_defaultgroupquota);
	if (error != 0)
		return (error);

	error = zfs_get_zplprop(os, ZFS_PROP_DEFAULTPROJECTQUOTA,
	    &zfsvfs->z_defaultprojectquota);
	if (error != 0)
		return (error);

	error = zfs_get_zplprop(os, ZFS_PROP_DEFAULTUSEROBJQUOTA,
	    &zfsvfs->z_defaultuserobjquota);
	if (error != 0)
		return (error);

	error = zfs_get_zplprop(os, ZFS_PROP_DEFAULTGROUPOBJQUOTA,
	    &zfsvfs->z_defaultgroupobjquota);
	if (error != 0)
		return (error);

	error = zfs_get_zplprop(os, ZFS_PROP_DEFAULTPROJECTOBJQUOTA,
	    &zfsvfs->z_defaultprojectobjquota);
	if (error != 0)
		return (error);

	error = sa_setup(os, sa_obj, zfs_attr_table, ZPL_END,
	    &zfsvfs->z_attr_table);
	if (error != 0)
		return (error);

	if (zfsvfs->z_version >= ZPL_VERSION_SA)
		sa_register_update_callback(os, zfs_sa_upgrade);

	error = zap_lookup(os, MASTER_NODE_OBJ, ZFS_ROOT_OBJ, 8, 1,
	    &zfsvfs->z_root);
	if (error != 0)
		return (error);
	ASSERT3U(zfsvfs->z_root, !=, 0);

	error = zap_lookup(os, MASTER_NODE_OBJ, ZFS_UNLINKED_SET, 8, 1,
	    &zfsvfs->z_unlinkedobj);
	if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ,
	    zfs_userquota_prop_prefixes[ZFS_PROP_USERQUOTA],
	    8, 1, &zfsvfs->z_userquota_obj);
	if (error == ENOENT)
		zfsvfs->z_userquota_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ,
	    zfs_userquota_prop_prefixes[ZFS_PROP_GROUPQUOTA],
	    8, 1, &zfsvfs->z_groupquota_obj);
	if (error == ENOENT)
		zfsvfs->z_groupquota_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ,
	    zfs_userquota_prop_prefixes[ZFS_PROP_PROJECTQUOTA],
	    8, 1, &zfsvfs->z_projectquota_obj);
	if (error == ENOENT)
		zfsvfs->z_projectquota_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ,
	    zfs_userquota_prop_prefixes[ZFS_PROP_USEROBJQUOTA],
	    8, 1, &zfsvfs->z_userobjquota_obj);
	if (error == ENOENT)
		zfsvfs->z_userobjquota_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ,
	    zfs_userquota_prop_prefixes[ZFS_PROP_GROUPOBJQUOTA],
	    8, 1, &zfsvfs->z_groupobjquota_obj);
	if (error == ENOENT)
		zfsvfs->z_groupobjquota_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ,
	    zfs_userquota_prop_prefixes[ZFS_PROP_PROJECTOBJQUOTA],
	    8, 1, &zfsvfs->z_projectobjquota_obj);
	if (error == ENOENT)
		zfsvfs->z_projectobjquota_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ, ZFS_FUID_TABLES, 8, 1,
	    &zfsvfs->z_fuid_obj);
	if (error == ENOENT)
		zfsvfs->z_fuid_obj = 0;
	else if (error != 0)
		return (error);

	error = zap_lookup(os, MASTER_NODE_OBJ, ZFS_SHARES_DIR, 8, 1,
	    &zfsvfs->z_shares_dir);
	if (error == ENOENT)
		zfsvfs->z_shares_dir = 0;
	else if (error != 0)
		return (error);

	return (0);
}

int
zfsvfs_create(const char *osname, boolean_t readonly, zfsvfs_t **zfvp)
{
	objset_t *os;
	zfsvfs_t *zfsvfs;
	int error;
	boolean_t ro = (readonly || (strchr(osname, '@') != NULL));

	zfsvfs = kmem_zalloc(sizeof (zfsvfs_t), KM_SLEEP);

	error = dmu_objset_own(osname, DMU_OST_ZFS, ro, B_TRUE, zfsvfs, &os);
	if (error != 0) {
		kmem_free(zfsvfs, sizeof (zfsvfs_t));
		return (error);
	}

	error = zfsvfs_create_impl(zfvp, zfsvfs, os);
	if (error != 0) {
		dmu_objset_disown(os, B_TRUE, zfsvfs);
	}
	return (error);
}


int
zfsvfs_create_impl(zfsvfs_t **zfvp, zfsvfs_t *zfsvfs, objset_t *os)
{
	int error;

	zfsvfs->z_vfs = NULL;
	zfsvfs->z_parent = zfsvfs;

	mutex_init(&zfsvfs->z_znodes_lock, NULL, MUTEX_DEFAULT, NULL);
	mutex_init(&zfsvfs->z_lock, NULL, MUTEX_DEFAULT, NULL);
	list_create(&zfsvfs->z_all_znodes, sizeof (znode_t),
	    offsetof(znode_t, z_link_node));
	ZFS_TEARDOWN_INIT(zfsvfs);
	ZFS_TEARDOWN_INACTIVE_INIT(zfsvfs);
	rw_init(&zfsvfs->z_fuid_lock, NULL, RW_DEFAULT, NULL);
	for (int i = 0; i != ZFS_OBJ_MTX_SZ; i++)
		mutex_init(&zfsvfs->z_hold_mtx[i], NULL, MUTEX_DEFAULT, NULL);

	error = zfsvfs_init(zfsvfs, os);
	if (error != 0) {
		*zfvp = NULL;
		kmem_free(zfsvfs, sizeof (zfsvfs_t));
		return (error);
	}

	zfsvfs->z_drain_task = TASKQID_INVALID;
	zfsvfs->z_draining = B_FALSE;
	zfsvfs->z_drain_cancel = B_TRUE;

	*zfvp = zfsvfs;
	return (0);
}

static int
zfsvfs_setup(zfsvfs_t *zfsvfs, boolean_t mounting)
{
	int error;

	/*
	 * Check for a bad on-disk format version now since we
	 * lied about owning the dataset readonly before.
	 */
	if (!(zfsvfs->z_vfs->vfs_flag & VFS_RDONLY) &&
	    dmu_objset_incompatible_encryption_version(zfsvfs->z_os))
		return (SET_ERROR(EROFS));

	error = zfs_register_callbacks(zfsvfs->z_vfs);
	if (error)
		return (error);

	/*
	 * If we are not mounting (ie: online recv), then we don't
	 * have to worry about replaying the log as we blocked all
	 * operations out since we closed the ZIL.
	 */
	if (mounting) {
		boolean_t readonly;

		ASSERT0P(zfsvfs->z_kstat.dk_kstats);
		error = dataset_kstats_create(&zfsvfs->z_kstat, zfsvfs->z_os);
		if (error)
			return (error);
		zfsvfs->z_log = zil_open(zfsvfs->z_os, zfs_get_data,
		    &zfsvfs->z_kstat.dk_zil_sums);

		/*
		 * During replay we remove the read only flag to
		 * allow replays to succeed.
		 */
		readonly = zfsvfs->z_vfs->vfs_flag & VFS_RDONLY;
		if (readonly != 0) {
			zfsvfs->z_vfs->vfs_flag &= ~VFS_RDONLY;
		} else {
			zfs_unlinked_drain(zfsvfs);
		}

		/*
		 * Parse and replay the intent log.
		 *
		 * Because of ziltest, this must be done after
		 * zfs_unlinked_drain().  (Further note: ziltest
		 * doesn't use readonly mounts, where
		 * zfs_unlinked_drain() isn't called.)  This is because
		 * ziltest causes spa_sync() to think it's committed,
		 * but actually it is not, so the intent log contains
		 * many txg's worth of changes.
		 *
		 * In particular, if object N is in the unlinked set in
		 * the last txg to actually sync, then it could be
		 * actually freed in a later txg and then reallocated
		 * in a yet later txg.  This would write a "create
		 * object N" record to the intent log.  Normally, this
		 * would be fine because the spa_sync() would have
		 * written out the fact that object N is free, before
		 * we could write the "create object N" intent log
		 * record.
		 *
		 * But when we are in ziltest mode, we advance the "open
		 * txg" without actually spa_sync()-ing the changes to
		 * disk.  So we would see that object N is still
		 * allocated and in the unlinked set, and there is an
		 * intent log record saying to allocate it.
		 */
		if (spa_writeable(dmu_objset_spa(zfsvfs->z_os))) {
			if (zil_replay_disable) {
				zil_destroy(zfsvfs->z_log, B_FALSE);
			} else {
				zfsvfs->z_replay = B_TRUE;
				zil_replay(zfsvfs->z_os, zfsvfs,
				    zfs_replay_vector);
				zfsvfs->z_replay = B_FALSE;
			}
		}

		/* restore readonly bit */
		if (readonly != 0)
			zfsvfs->z_vfs->vfs_flag |= VFS_RDONLY;
	} else {
		ASSERT3P(zfsvfs->z_kstat.dk_kstats, !=, NULL);
		zfsvfs->z_log = zil_open(zfsvfs->z_os, zfs_get_data,
		    &zfsvfs->z_kstat.dk_zil_sums);
	}

	/*
	 * Set the objset user_ptr to track its zfsvfs.
	 */
	mutex_enter(&zfsvfs->z_os->os_user_ptr_lock);
	dmu_objset_set_user(zfsvfs->z_os, zfsvfs);
	mutex_exit(&zfsvfs->z_os->os_user_ptr_lock);

	return (0);
}

void
zfsvfs_free(zfsvfs_t *zfsvfs)
{
	int i;

	zfs_fuid_destroy(zfsvfs);

	mutex_destroy(&zfsvfs->z_znodes_lock);
	mutex_destroy(&zfsvfs->z_lock);
	list_destroy(&zfsvfs->z_all_znodes);
	ZFS_TEARDOWN_DESTROY(zfsvfs);
	ZFS_TEARDOWN_INACTIVE_DESTROY(zfsvfs);
	rw_destroy(&zfsvfs->z_fuid_lock);
	for (i = 0; i != ZFS_OBJ_MTX_SZ; i++)
		mutex_destroy(&zfsvfs->z_hold_mtx[i]);
	dataset_kstats_destroy(&zfsvfs->z_kstat);
	kmem_free(zfsvfs, sizeof (zfsvfs_t));
}

static void
zfs_set_fuid_feature(zfsvfs_t *zfsvfs)
{
	zfsvfs->z_use_fuids = USE_FUIDS(zfsvfs->z_version, zfsvfs->z_os);
	if (zfsvfs->z_vfs) {
		if (zfsvfs->z_use_fuids) {
			vfs_set_feature(zfsvfs->z_vfs, VFSFT_XVATTR);
			vfs_set_feature(zfsvfs->z_vfs, VFSFT_SYSATTR_VIEWS);
			vfs_set_feature(zfsvfs->z_vfs, VFSFT_ACEMASKONACCESS);
			vfs_set_feature(zfsvfs->z_vfs, VFSFT_ACLONCREATE);
			vfs_set_feature(zfsvfs->z_vfs, VFSFT_ACCESS_FILTER);
			vfs_set_feature(zfsvfs->z_vfs, VFSFT_REPARSE);
		} else {
			vfs_clear_feature(zfsvfs->z_vfs, VFSFT_XVATTR);
			vfs_clear_feature(zfsvfs->z_vfs, VFSFT_SYSATTR_VIEWS);
			vfs_clear_feature(zfsvfs->z_vfs,
			    VFSFT_ACEMASKONACCESS);
			vfs_clear_feature(zfsvfs->z_vfs, VFSFT_ACLONCREATE);
			vfs_clear_feature(zfsvfs->z_vfs, VFSFT_ACCESS_FILTER);
			vfs_clear_feature(zfsvfs->z_vfs, VFSFT_REPARSE);
		}
	}
	zfsvfs->z_use_sa = USE_SA(zfsvfs->z_version, zfsvfs->z_os);
}

static int
zfs_domount(vfs_t *vfsp, char *osname)
{
	dev_t mount_dev;
	uint64_t recordsize, fsid_guid;
	int error = 0;
	zfsvfs_t *zfsvfs;
	boolean_t readonly = vfsp->vfs_flag & VFS_RDONLY ? B_TRUE : B_FALSE;

	ASSERT(vfsp);
	ASSERT(osname);

	error = zfsvfs_create(osname, readonly, &zfsvfs);
	if (error)
		return (error);
	zfsvfs->z_vfs = vfsp;

	/* Initialize the generic filesystem structure. */
	vfsp->vfs_bcount = 0;
	vfsp->vfs_data = NULL;

	if (zfs_create_unique_device(&mount_dev) == -1) {
		error = SET_ERROR(ENODEV);
		goto out;
	}
	ASSERT(vfs_devismounted(mount_dev) == 0);

	if ((error = dsl_prop_get_integer(osname, "recordsize", &recordsize,
	    NULL)) != 0)
		goto out;

	vfsp->vfs_dev = mount_dev;
	vfsp->vfs_fstype = zfsfstype;
	vfsp->vfs_bsize = recordsize;
	vfsp->vfs_flag |= VFS_NOTRUNC;
	vfsp->vfs_data = zfsvfs;

	/*
	 * The fsid is 64 bits, composed of an 8-bit fs type, which
	 * separates our fsid from any other filesystem types, and a
	 * 56-bit objset unique ID.  The objset unique ID is unique to
	 * all objsets open on this system, provided by unique_create().
	 * The 8-bit fs type must be put in the low bits of fsid[1]
	 * because that's where other Solaris filesystems put it.
	 */
	fsid_guid = dmu_objset_fsid_guid(zfsvfs->z_os);
	ASSERT((fsid_guid & ~((1ULL << 56) - 1)) == 0);
	vfsp->vfs_fsid.val[0] = fsid_guid;
	vfsp->vfs_fsid.val[1] = ((fsid_guid >> 32) << 8) |
	    zfsfstype & 0xFF;

	/*
	 * Set features for file system.
	 */
	zfs_set_fuid_feature(zfsvfs);
	if (zfsvfs->z_case == ZFS_CASE_INSENSITIVE) {
		vfs_set_feature(vfsp, VFSFT_DIRENTFLAGS);
		vfs_set_feature(vfsp, VFSFT_CASEINSENSITIVE);
		vfs_set_feature(vfsp, VFSFT_NOCASESENSITIVE);
	} else if (zfsvfs->z_case == ZFS_CASE_MIXED) {
		vfs_set_feature(vfsp, VFSFT_DIRENTFLAGS);
		vfs_set_feature(vfsp, VFSFT_CASEINSENSITIVE);
	}
	vfs_set_feature(vfsp, VFSFT_ZEROCOPY_SUPPORTED);

	if (dmu_objset_is_snapshot(zfsvfs->z_os)) {
		uint64_t pval;

		atime_changed_cb(zfsvfs, B_FALSE);
		readonly_changed_cb(zfsvfs, B_TRUE);
		if ((error = dsl_prop_get_integer(osname, "xattr", &pval,
		    NULL)) != 0)
			goto out;
		xattr_changed_cb(zfsvfs, pval);
		zfsvfs->z_issnap = B_TRUE;
		zfsvfs->z_os->os_sync = ZFS_SYNC_DISABLED;

		mutex_enter(&zfsvfs->z_os->os_user_ptr_lock);
		dmu_objset_set_user(zfsvfs->z_os, zfsvfs);
		mutex_exit(&zfsvfs->z_os->os_user_ptr_lock);
	} else {
		error = zfsvfs_setup(zfsvfs, B_TRUE);
	}

	/* cache the root vnode for this mount */
	if (error == 0) {
		znode_t *rootzp;
		if ((error = zfs_zget(zfsvfs, zfsvfs->z_root,
		    &rootzp)) != 0) {
			goto out;
		}
		zfsvfs->z_rootdir = ZTOV(rootzp);
	}

	if (!zfsvfs->z_issnap)
		zfsctl_create(zfsvfs);
out:
	if (error) {
		dmu_objset_disown(zfsvfs->z_os, B_TRUE, zfsvfs);
		zfsvfs_free(zfsvfs);
	} else {
		atomic_inc_32(&zfs_active_fs_count);
	}

	return (error);
}

void
zfs_unregister_callbacks(zfsvfs_t *zfsvfs)
{
	objset_t *os = zfsvfs->z_os;

	if (!dmu_objset_is_snapshot(os))
		dsl_prop_unregister_all(dmu_objset_ds(os), zfsvfs);
}

/*
 * zfs_mount - stub for the full mount(2) path.
 *
 * The full illumos mount(2) entry point handles privilege checks,
 * zone visibility, label policies, remounts, and the mounta struct.
 * For the initial kernel port we provide a minimal stub that calls
 * zfs_domount() so that pool import (which calls zfs_domount()
 * directly) continues to work.
 */
/*ARGSUSED*/
static int
zfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	char		*osname;
	pathname_t	spn;
	int		error = 0;
	uio_seg_t	fromspace = (uap->flags & MS_SYSSPACE) ?
	    UIO_SYSSPACE : UIO_USERSPACE;
	int		canwrite;

	if (mvp->v_type != VDIR)
		return (SET_ERROR(ENOTDIR));

	mutex_enter(&mvp->v_lock);
	if ((uap->flags & MS_REMOUNT) == 0 &&
	    (uap->flags & MS_OVERLAY) == 0 &&
	    (vn_count(mvp) != 1 || (mvp->v_flag & VROOT))) {
		mutex_exit(&mvp->v_lock);
		return (SET_ERROR(EBUSY));
	}
	mutex_exit(&mvp->v_lock);

	/*
	 * ZFS does not support passing unparsed data in via MS_DATA.
	 * Users should use the MS_OPTIONSTR interface; this means
	 * that all option parsing is already done and the options struct
	 * can be interrogated.
	 */
	if ((uap->flags & MS_DATA) && uap->datalen > 0)
		return (SET_ERROR(EINVAL));

	/*
	 * Get the objset name (the "special" mount argument).
	 */
	if ((error = pn_get(uap->spec, fromspace, &spn)) != 0)
		return (error);

	osname = spn.pn_buf;

	/*
	 * Check for mount privilege?
	 *
	 * If we don't have privilege then see if
	 * we have local permission to allow it
	 */
	error = secpolicy_fs_mount(cr, mvp, vfsp);
	if (error) {
		if (dsl_deleg_access(osname, ZFS_DELEG_PERM_MOUNT, cr) == 0) {
			vattr_t		vattr;

			/*
			 * Make sure user is the owner of the mount point
			 * or has sufficient privileges.
			 */
			vattr.va_mask = AT_UID;

			if (VOP_GETATTR(mvp, &vattr, 0, cr, NULL)) {
				goto out;
			}

			if (secpolicy_vnode_owner(cr, vattr.va_uid) != 0 &&
			    VOP_ACCESS(mvp, VWRITE, 0, cr, NULL) != 0) {
				goto out;
			}
			secpolicy_fs_mount_clearopts(cr, vfsp);
		} else {
			goto out;
		}
	}

	/*
	 * Refuse to mount a filesystem if we are in a local zone and the
	 * dataset is not visible.
	 */
	if (!INGLOBALZONE(curproc) &&
	    (!zone_dataset_visible(osname, &canwrite) || !canwrite)) {
		error = SET_ERROR(EPERM);
		goto out;
	}

	/*
	 * When doing a remount, we simply refresh our temporary properties
	 * according to those options set in the current VFS options.
	 */
	if (uap->flags & MS_REMOUNT) {
		/* refresh mount options */
		zfs_unregister_callbacks(vfsp->vfs_data);
		error = zfs_register_callbacks(vfsp);
		goto out;
	}

	error = zfs_domount(vfsp, osname);

	/*
	 * Add an extra VFS_HOLD on our parent vfs so that it can't
	 * disappear due to a forced unmount.
	 */
	if (error == 0 && ((zfsvfs_t *)vfsp->vfs_data)->z_issnap)
		VFS_HOLD(mvp->v_vfsp);

out:
	pn_free(&spn);
	return (error);
}

static int
zfs_statvfs(vfs_t *vfsp, struct statvfs64 *statp)
{
	zfsvfs_t *zfsvfs = vfsp->vfs_data;
	dev32_t d32;
	uint64_t refdbytes, availbytes, usedobjs, availobjs;
	int err = 0;

	if ((err = zfs_enter(zfsvfs, FTAG)) != 0)
		return (err);

	dmu_objset_space(zfsvfs->z_os,
	    &refdbytes, &availbytes, &usedobjs, &availobjs);

	/*
	 * The underlying storage pool actually uses multiple block sizes.
	 * We report the fragsize as the smallest block size we support,
	 * and we report our blocksize as the filesystem's maximum blocksize.
	 */
	statp->f_frsize = 1UL << SPA_MINBLOCKSHIFT;
	statp->f_bsize = zfsvfs->z_max_blksz;

	/*
	 * The following report "total" blocks of various kinds in the
	 * file system, but reported in terms of f_frsize - the
	 * "fragment" size.
	 */
	statp->f_blocks = (refdbytes + availbytes) >> SPA_MINBLOCKSHIFT;
	statp->f_bfree = availbytes >> SPA_MINBLOCKSHIFT;
	statp->f_bavail = statp->f_bfree; /* no root reservation */

	/*
	 * statvfs() should really be called statufs(), because it assumes
	 * static metadata.  ZFS doesn't preallocate files, so the best
	 * we can do is report the max that could possibly fit in f_files,
	 * and that minus the number actually used in f_ffree.
	 * For f_ffree, report the smaller of the number of object available
	 * and the number of blocks (each object will take at least a block).
	 */
	statp->f_ffree = MIN(availobjs, statp->f_bfree);
	statp->f_favail = statp->f_ffree;	/* no "root reservation" */
	statp->f_files = statp->f_ffree + usedobjs;

	(void) cmpldev(&d32, vfsp->vfs_dev);
	statp->f_fsid = d32;

	/*
	 * We're a zfs filesystem.
	 */
	(void) strcpy(statp->f_basetype,
	    vfssw[vfsp->vfs_fstype].vsw_name);

	statp->f_flag = vf_to_stf(vfsp->vfs_flag);

	statp->f_namemax = zfsvfs->z_longname ?
	    (ZAP_MAXNAMELEN_NEW - 1) : (MAXNAMELEN - 1);

	/*
	 * We have all of 32 characters to stuff a string here.
	 * Is there anything useful we could/should provide?
	 */
	bzero(statp->f_fstr, sizeof (statp->f_fstr));

	zfs_exit(zfsvfs, FTAG);
	return (err);
}

static int
zfs_root(vfs_t *vfsp, vnode_t **vpp)
{
	zfsvfs_t *zfsvfs = vfsp->vfs_data;
	vnode_t *vp;
	int error;

	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		return (error);

	vp = zfsvfs->z_rootdir;
	if (vp != NULL) {
		VN_HOLD(vp);
		error = 0;
	} else {
		/* forced unmount */
		error = EIO;
	}
	*vpp = vp;

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Teardown the zfsvfs::z_os.
 *
 * Note, if 'unmounting' is FALSE, we return with the 'z_teardown_lock'
 * and 'z_teardown_inactive_lock' held.
 */
static int
zfsvfs_teardown(zfsvfs_t *zfsvfs, boolean_t unmounting)
{
	znode_t	*zp;

	ZFS_TEARDOWN_ENTER_WRITE(zfsvfs, FTAG);

	if (!unmounting) {
		/*
		 * We purge the parent filesystem's vfsp as the parent
		 * filesystem and all of its snapshots have their vnode's
		 * v_vfsp set to the parent's filesystem's vfsp.  Note,
		 * 'z_parent' is self referential for non-snapshots.
		 */
		(void) dnlc_purge_vfsp(zfsvfs->z_parent->z_vfs, 0);
	}

	/*
	 * Close the zil. NB: Can't close the zil while zfs_inactive
	 * threads are blocked as zil_close can call zfs_inactive.
	 */
	if (zfsvfs->z_log) {
		zil_close(zfsvfs->z_log);
		zfsvfs->z_log = NULL;
	}

	ZFS_TEARDOWN_INACTIVE_ENTER_WRITE(zfsvfs);

	/*
	 * If we are not unmounting (ie: online recv) and someone already
	 * unmounted this file system while we were doing the switcheroo,
	 * or a reopen of z_os failed then just bail out now.
	 */
	if (!unmounting && (zfsvfs->z_unmounted || zfsvfs->z_os == NULL)) {
		ZFS_TEARDOWN_INACTIVE_EXIT_WRITE(zfsvfs);
		ZFS_TEARDOWN_EXIT(zfsvfs, FTAG);
		return (SET_ERROR(EIO));
	}

	/*
	 * At this point there are no vops active, and any new vops will
	 * fail with EIO since we have z_teardown_lock for writer (only
	 * relevant for forced unmount).
	 *
	 * Release all holds on dbufs.
	 */
	mutex_enter(&zfsvfs->z_znodes_lock);
	for (zp = list_head(&zfsvfs->z_all_znodes); zp != NULL;
	    zp = list_next(&zfsvfs->z_all_znodes, zp))
		if (zp->z_sa_hdl) {
			ASSERT(ZTOV(zp)->v_count > 0);
			zfs_znode_dmu_fini(zp);
		}
	mutex_exit(&zfsvfs->z_znodes_lock);

	/*
	 * If we are unmounting, set the unmounted flag and let new vops
	 * unblock.  zfs_inactive will have the unmounted behavior, and all
	 * other vops will fail with EIO.
	 */
	if (unmounting) {
		/*
		 * Clear the cached root vnode now that we are unmounted.
		 * Its release must be performed outside the teardown locks to
		 * avoid recursive lock entry via zfs_inactive().
		 */
		vnode_t *vp = zfsvfs->z_rootdir;
		zfsvfs->z_rootdir = NULL;

		zfsvfs->z_unmounted = B_TRUE;
		ZFS_TEARDOWN_INACTIVE_EXIT_WRITE(zfsvfs);
		ZFS_TEARDOWN_EXIT(zfsvfs, FTAG);

		/* Drop the cached root vp now that it is safe */
		if (vp != NULL)
			VN_RELE(vp);
	}

	/*
	 * z_os will be NULL if there was an error in attempting to reopen
	 * zfsvfs, so just return as the properties had already been
	 * unregistered and cached data had been evicted before.
	 */
	if (zfsvfs->z_os == NULL)
		return (0);

	/*
	 * Unregister properties.
	 */
	zfs_unregister_callbacks(zfsvfs);

	/*
	 * Evict cached data
	 */
	if (dsl_dataset_is_dirty(dmu_objset_ds(zfsvfs->z_os)) &&
	    !(zfsvfs->z_vfs->vfs_flag & VFS_RDONLY))
		txg_wait_synced(dmu_objset_pool(zfsvfs->z_os), 0);
	dmu_objset_evict_dbufs(zfsvfs->z_os);

	return (0);
}

/*ARGSUSED*/
static int
zfs_umount(vfs_t *vfsp, int fflag, cred_t *cr)
{
	zfsvfs_t *zfsvfs = vfsp->vfs_data;
	objset_t *os;
	int ret;

	ret = secpolicy_fs_unmount(cr, vfsp);
	if (ret) {
		if (dsl_deleg_access((char *)refstr_value(vfsp->vfs_resource),
		    ZFS_DELEG_PERM_MOUNT, cr))
			return (ret);
	}

	/*
	 * We purge the parent filesystem's vfsp as the parent filesystem
	 * and all of its snapshots have their vnode's v_vfsp set to the
	 * parent's filesystem's vfsp.  Note, 'z_parent' is self
	 * referential for non-snapshots.
	 */
	(void) dnlc_purge_vfsp(zfsvfs->z_parent->z_vfs, 0);

	/*
	 * Unmount any snapshots mounted under .zfs before unmounting the
	 * dataset itself.
	 */
	if (zfsvfs->z_ctldir != NULL &&
	    (ret = zfsctl_umount_snapshots(vfsp, fflag, cr)) != 0) {
		return (ret);
	}

	if (!(fflag & MS_FORCE)) {
		/*
		 * Check the number of active vnodes in the file system.
		 * Our count is maintained in the vfs structure, but the
		 * number is off by 1 to indicate a hold on the vfs
		 * structure itself.
		 *
		 * The '.zfs' directory maintains a reference of its own,
		 * and any active references underneath are reflected in the
		 * vnode count.
		 */
		uint_t thresh = 1;
		vnode_t *rvp, *ctlvp;

		/* The cached root vnode also maintains a hold */
		rvp = zfsvfs->z_rootdir;
		thresh++;

		ctlvp = zfsvfs->z_ctldir;
		if (ctlvp != NULL)
			thresh++;

		if (vfsp->vfs_count > thresh || rvp->v_count > 1 ||
		    (ctlvp != NULL && ctlvp->v_count > 1)) {
			return (SET_ERROR(EBUSY));
		}
	}

	vfsp->vfs_flag |= VFS_UNMOUNTED;

	VERIFY(zfsvfs_teardown(zfsvfs, B_TRUE) == 0);
	os = zfsvfs->z_os;

	/*
	 * z_os will be NULL if there was an error in
	 * attempting to reopen zfsvfs.
	 */
	if (os != NULL) {
		/*
		 * Unset the objset user_ptr.
		 */
		mutex_enter(&os->os_user_ptr_lock);
		dmu_objset_set_user(os, NULL);
		mutex_exit(&os->os_user_ptr_lock);

		/*
		 * Finally release the objset
		 */
		dmu_objset_disown(os, B_TRUE, zfsvfs);
	}

	/*
	 * We can now safely destroy the '.zfs' directory node.
	 */
	if (zfsvfs->z_ctldir != NULL)
		zfsctl_destroy(zfsvfs);

	return (0);
}

static int
zfs_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
{
	zfsvfs_t	*zfsvfs = vfsp->vfs_data;
	znode_t		*zp;
	uint64_t	object = 0;
	uint64_t	fid_gen = 0;
	uint64_t	gen_mask;
	uint64_t	zp_gen;
	int		i, err;

	*vpp = NULL;

	if ((err = zfs_enter(zfsvfs, FTAG)) != 0)
		return (err);

	if (fidp->fid_len == LONG_FID_LEN) {
		zfid_long_t	*zlfid = (zfid_long_t *)fidp;
		uint64_t	objsetid = 0;
		uint64_t	setgen = 0;

		for (i = 0; i < sizeof (zlfid->zf_setid); i++)
			objsetid |= ((uint64_t)zlfid->zf_setid[i]) << (8 * i);

		for (i = 0; i < sizeof (zlfid->zf_setgen); i++)
			setgen |= ((uint64_t)zlfid->zf_setgen[i]) << (8 * i);

		zfs_exit(zfsvfs, FTAG);

		err = zfsctl_lookup_objset(vfsp, objsetid, &zfsvfs);
		if (err)
			return (SET_ERROR(EINVAL));
		if ((err = zfs_enter(zfsvfs, FTAG)) != 0)
			return (err);
	}

	if (fidp->fid_len == SHORT_FID_LEN || fidp->fid_len == LONG_FID_LEN) {
		zfid_short_t	*zfid = (zfid_short_t *)fidp;

		for (i = 0; i < sizeof (zfid->zf_object); i++)
			object |= ((uint64_t)zfid->zf_object[i]) << (8 * i);

		for (i = 0; i < sizeof (zfid->zf_gen); i++)
			fid_gen |= ((uint64_t)zfid->zf_gen[i]) << (8 * i);
	} else {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	/* A zero fid_gen means we are in the .zfs control directories */
	if (fid_gen == 0 &&
	    (object == ZFSCTL_INO_ROOT || object == ZFSCTL_INO_SNAPDIR)) {
		*vpp = zfsvfs->z_ctldir;
		ASSERT(*vpp != NULL);
		if (object == ZFSCTL_INO_SNAPDIR) {
			VERIFY(zfsctl_root_lookup(*vpp, "snapshot", vpp, NULL,
			    0, NULL, NULL, NULL, NULL, NULL) == 0);
		} else {
			VN_HOLD(*vpp);
		}
		zfs_exit(zfsvfs, FTAG);
		return (0);
	}

	gen_mask = -1ULL >> (64 - 8 * i);

	if ((err = zfs_zget(zfsvfs, object, &zp)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (err);
	}
	(void) sa_lookup(zp->z_sa_hdl, SA_ZPL_GEN(zfsvfs), &zp_gen,
	    sizeof (uint64_t));
	zp_gen = zp_gen & gen_mask;
	if (zp_gen == 0)
		zp_gen = 1;
	if (zp->z_unlinked || zp_gen != fid_gen) {
		VN_RELE(ZTOV(zp));
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	*vpp = ZTOV(zp);
	zfs_exit(zfsvfs, FTAG);
	return (0);
}

/*
 * Block out VOPs and close zfsvfs_t::z_os
 *
 * Note, if successful, then we return with the 'z_teardown_lock' and
 * 'z_teardown_inactive_lock' write held.  We leave ownership of the underlying
 * dataset and objset intact so that they can be atomically handed off during
 * a subsequent rollback or recv operation and the resume thereafter.
 */
int
zfs_suspend_fs(zfsvfs_t *zfsvfs)
{
	int error;

	if ((error = zfsvfs_teardown(zfsvfs, B_FALSE)) != 0)
		return (error);

	return (0);
}

/*
 * Rebuild SA and release VOPs.  Note that ownership of the underlying dataset
 * is an invariant across any of the operations that can be performed while the
 * filesystem was suspended.  Whether it succeeded or failed, the preconditions
 * are the same: the relevant objset and associated dataset are owned by
 * zfsvfs, held, and long held on entry.
 */
int
zfs_resume_fs(zfsvfs_t *zfsvfs, dsl_dataset_t *ds)
{
	int err;
	znode_t *zp;

	ASSERT(ZFS_TEARDOWN_WRITE_HELD(zfsvfs));
	ASSERT(ZFS_TEARDOWN_INACTIVE_WRITE_HELD(zfsvfs));

	/*
	 * We already own this, so just update the objset_t, as the one we
	 * had before may have been evicted.
	 */
	objset_t *os;
	VERIFY3P(ds->ds_owner, ==, zfsvfs);
	VERIFY(dsl_dataset_long_held(ds));
	dsl_pool_t *dp = spa_get_dsl(dsl_dataset_get_spa(ds));
	dsl_pool_config_enter(dp, FTAG);
	VERIFY0(dmu_objset_from_ds(ds, &os));
	dsl_pool_config_exit(dp, FTAG);

	err = zfsvfs_init(zfsvfs, os);
	if (err != 0)
		goto bail;

	ds->ds_dir->dd_activity_cancelled = B_FALSE;
	VERIFY0(zfsvfs_setup(zfsvfs, B_FALSE));

	zfs_set_fuid_feature(zfsvfs);

	/*
	 * Attempt to re-establish all the active znodes with
	 * their dbufs.  If a zfs_rezget() fails, then we'll let
	 * any potential callers discover that via zfs_enter_verify_zp
	 * when they try to use their znode.
	 */
	mutex_enter(&zfsvfs->z_znodes_lock);
	for (zp = list_head(&zfsvfs->z_all_znodes); zp;
	    zp = list_next(&zfsvfs->z_all_znodes, zp)) {
		(void) zfs_rezget(zp);
	}
	mutex_exit(&zfsvfs->z_znodes_lock);

bail:
	/* release the VOPs */
	ZFS_TEARDOWN_INACTIVE_EXIT_WRITE(zfsvfs);
	ZFS_TEARDOWN_EXIT(zfsvfs, FTAG);

	if (err) {
		/*
		 * Since we couldn't setup the sa framework, try to force
		 * unmount this file system.
		 */
		if (vn_vfswlock(zfsvfs->z_vfs->vfs_vnodecovered) == 0) {
			vfs_ref(zfsvfs->z_vfs);
			(void) dounmount(zfsvfs->z_vfs, MS_FORCE, CRED());
		}
	}
	return (err);
}

/*
 * Release VOPs and unmount a suspended filesystem.
 */
int
zfs_end_fs(zfsvfs_t *zfsvfs, dsl_dataset_t *ds)
{
	ASSERT(ZFS_TEARDOWN_WRITE_HELD(zfsvfs));
	ASSERT(ZFS_TEARDOWN_INACTIVE_WRITE_HELD(zfsvfs));

	/*
	 * We already own this, so just hold and rele it to update the
	 * objset_t, as the one we had before may have been evicted.
	 */
	objset_t *os;
	VERIFY3P(ds->ds_owner, ==, zfsvfs);
	VERIFY(dsl_dataset_long_held(ds));
	dsl_pool_t *dp = spa_get_dsl(dsl_dataset_get_spa(ds));
	dsl_pool_config_enter(dp, FTAG);
	VERIFY0(dmu_objset_from_ds(ds, &os));
	dsl_pool_config_exit(dp, FTAG);
	zfsvfs->z_os = os;

	/* release the VOPs */
	ZFS_TEARDOWN_INACTIVE_EXIT_WRITE(zfsvfs);
	ZFS_TEARDOWN_EXIT(zfsvfs, FTAG);

	/*
	 * Try to force unmount this file system.
	 */
	(void) zfs_umount(zfsvfs->z_vfs, 0, CRED());
	zfsvfs->z_unmounted = B_TRUE;
	return (0);
}

static void
zfs_freevfs(vfs_t *vfsp)
{
	zfsvfs_t *zfsvfs = vfsp->vfs_data;

	zfsvfs_free(zfsvfs);

	atomic_dec_32(&zfs_active_fs_count);
}

boolean_t
zfs_is_readonly(zfsvfs_t *zfsvfs)
{
	return (!!(zfsvfs->z_vfs->vfs_flag & VFS_RDONLY));
}

int
zfs_get_temporary_prop(dsl_dataset_t *ds, zfs_prop_t zfs_prop, uint64_t *val,
    char *setpoint)
{
	int error;
	zfsvfs_t *zfvp;
	vfs_t *vfsp;
	objset_t *os;
	uint64_t tmp = *val;

	error = dmu_objset_from_ds(ds, &os);
	if (error != 0)
		return (error);

	error = getzfsvfs_impl(os, &zfvp);
	if (error != 0)
		return (error);
	if (zfvp == NULL)
		return (ENOENT);
	vfsp = zfvp->z_vfs;

	switch (zfs_prop) {
	case ZFS_PROP_ATIME:
		if (vfs_optionisset(vfsp, MNTOPT_NOATIME, NULL))
			tmp = 0;
		if (vfs_optionisset(vfsp, MNTOPT_ATIME, NULL))
			tmp = 1;
		break;
	case ZFS_PROP_DEVICES:
		if (vfs_optionisset(vfsp, MNTOPT_NODEVICES, NULL))
			tmp = 0;
		if (vfs_optionisset(vfsp, MNTOPT_DEVICES, NULL))
			tmp = 1;
		break;
	case ZFS_PROP_EXEC:
		if (vfs_optionisset(vfsp, MNTOPT_NOEXEC, NULL))
			tmp = 0;
		if (vfs_optionisset(vfsp, MNTOPT_EXEC, NULL))
			tmp = 1;
		break;
	case ZFS_PROP_SETUID:
		if (vfs_optionisset(vfsp, MNTOPT_NOSETUID, NULL))
			tmp = 0;
		if (vfs_optionisset(vfsp, MNTOPT_SETUID, NULL))
			tmp = 1;
		break;
	case ZFS_PROP_READONLY:
		if (vfs_optionisset(vfsp, MNTOPT_RW, NULL))
			tmp = 0;
		if (vfs_optionisset(vfsp, MNTOPT_RO, NULL))
			tmp = 1;
		break;
	case ZFS_PROP_XATTR:
		if (zfvp->z_flags & ZSB_XATTR)
			tmp = zfvp->z_xattr;
		break;
	case ZFS_PROP_NBMAND:
		if (vfs_optionisset(vfsp, MNTOPT_NONBMAND, NULL))
			tmp = 0;
		if (vfs_optionisset(vfsp, MNTOPT_NBMAND, NULL))
			tmp = 1;
		break;
	default:
		return (ENOENT);
	}

	if (tmp != *val) {
		if (setpoint)
			(void) strcpy(setpoint, "temporary");
		*val = tmp;
	}
	return (0);
}

int
zfs_set_version(zfsvfs_t *zfsvfs, uint64_t newvers)
{
	int error;
	objset_t *os = zfsvfs->z_os;
	dmu_tx_t *tx;

	if (newvers < ZPL_VERSION_INITIAL || newvers > ZPL_VERSION)
		return (SET_ERROR(EINVAL));

	if (newvers < zfsvfs->z_version)
		return (SET_ERROR(EINVAL));

	if (zfs_spa_version_map(newvers) >
	    spa_version(dmu_objset_spa(zfsvfs->z_os)))
		return (SET_ERROR(ENOTSUP));

	tx = dmu_tx_create(os);
	dmu_tx_hold_zap(tx, MASTER_NODE_OBJ, B_FALSE, ZPL_VERSION_STR);
	if (newvers >= ZPL_VERSION_SA && !zfsvfs->z_use_sa) {
		dmu_tx_hold_zap(tx, MASTER_NODE_OBJ, B_TRUE,
		    ZFS_SA_ATTRS);
		dmu_tx_hold_zap(tx, DMU_NEW_OBJECT, FALSE, NULL);
	}
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		dmu_tx_abort(tx);
		return (error);
	}

	error = zap_update(os, MASTER_NODE_OBJ, ZPL_VERSION_STR,
	    8, 1, &newvers, tx);

	if (error) {
		dmu_tx_commit(tx);
		return (error);
	}

	if (newvers >= ZPL_VERSION_SA && !zfsvfs->z_use_sa) {
		uint64_t sa_obj;

		ASSERT3U(spa_version(dmu_objset_spa(zfsvfs->z_os)), >=,
		    SPA_VERSION_SA);
		sa_obj = zap_create(os, DMU_OT_SA_MASTER_NODE,
		    DMU_OT_NONE, 0, tx);

		error = zap_add(os, MASTER_NODE_OBJ,
		    ZFS_SA_ATTRS, 8, 1, &sa_obj, tx);
		ASSERT0(error);

		VERIFY0(sa_set_sa_object(os, sa_obj));
		sa_register_update_callback(os, zfs_sa_upgrade);
	}

	spa_history_log_internal_ds(dmu_objset_ds(os), "upgrade", tx,
	    "from %ju to %ju", (uintmax_t)zfsvfs->z_version,
	    (uintmax_t)newvers);
	dmu_tx_commit(tx);

	zfsvfs->z_version = newvers;
	os->os_version = newvers;

	zfs_set_fuid_feature(zfsvfs);

	return (0);
}

int
zfs_set_default_quota(zfsvfs_t *zfsvfs, zfs_prop_t prop, uint64_t quota)
{
	int error;
	objset_t *os = zfsvfs->z_os;
	const char *propstr = zfs_prop_to_name(prop);
	dmu_tx_t *tx;

	tx = dmu_tx_create(os);
	dmu_tx_hold_zap(tx, MASTER_NODE_OBJ, B_FALSE, propstr);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		dmu_tx_abort(tx);
		return (error);
	}

	if (quota == 0) {
		error = zap_remove(os, MASTER_NODE_OBJ, propstr, tx);
		if (error == ENOENT)
			error = 0;
	} else {
		error = zap_update(os, MASTER_NODE_OBJ, propstr, 8, 1,
		    &quota, tx);
	}

	if (error)
		goto out;

	switch (prop) {
	case ZFS_PROP_DEFAULTUSERQUOTA:
		zfsvfs->z_defaultuserquota = quota;
		break;
	case ZFS_PROP_DEFAULTGROUPQUOTA:
		zfsvfs->z_defaultgroupquota = quota;
		break;
	case ZFS_PROP_DEFAULTPROJECTQUOTA:
		zfsvfs->z_defaultprojectquota = quota;
		break;
	case ZFS_PROP_DEFAULTUSEROBJQUOTA:
		zfsvfs->z_defaultuserobjquota = quota;
		break;
	case ZFS_PROP_DEFAULTGROUPOBJQUOTA:
		zfsvfs->z_defaultgroupobjquota = quota;
		break;
	case ZFS_PROP_DEFAULTPROJECTOBJQUOTA:
		zfsvfs->z_defaultprojectobjquota = quota;
		break;
	default:
		break;
	}

out:
	dmu_tx_commit(tx);
	return (error);
}

/*
 * Return true if the corresponding vfs's unmounted flag is set.
 * Otherwise return false.
 * If this function returns true we know VFS unmount has been initiated.
 */
boolean_t
zfs_get_vfs_flag_unmounted(objset_t *os)
{
	zfsvfs_t *zfvp;
	boolean_t unmounted = B_FALSE;

	ASSERT3U(dmu_objset_type(os), ==, DMU_OST_ZFS);

	mutex_enter(&os->os_user_ptr_lock);
	zfvp = dmu_objset_get_user(os);
	if (zfvp != NULL && zfvp->z_vfs != NULL &&
	    (zfvp->z_vfs->vfs_flag & VFS_UNMOUNTED))
		unmounted = B_TRUE;
	mutex_exit(&os->os_user_ptr_lock);

	return (unmounted);
}

void
zfsvfs_update_fromname(const char *oldname, const char *newname)
{
	/*
	 * On illumos, vfs_resource is a refstr; we would need to iterate
	 * the mount list and update matching entries.  For now this is a
	 * stub -- rename of mounted datasets will update on next mount.
	 */
}

void
zfs_init(void)
{
	/*
	 * Initialize .zfs directory structures
	 */
	zfsctl_init();

	/*
	 * Initialize znode cache, vnode ops, etc...
	 */
	zfs_znode_init();

	dmu_objset_register_type(DMU_OST_ZFS, zpl_get_file_info);

	mutex_init(&zfs_dev_mtx, NULL, MUTEX_DEFAULT, NULL);
}

void
zfs_fini(void)
{
	mutex_destroy(&zfs_dev_mtx);

	zfsctl_fini();
	zfs_znode_fini();
}

int
zfs_busy(void)
{
	return (zfs_active_fs_count != 0);
}
