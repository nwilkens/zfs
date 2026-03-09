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
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * OS-specific ioctl support for illumos.
 *
 * This file provides:
 *   - zfsdev_open/close/ioctl: /dev/zfs character device entry points
 *   - zfs_vfs_ref/rele/held: VFS reference counting for mounted filesystems
 *   - zfsdev_attach/detach: device creation/destruction
 *   - zfsdev_private_set_state/get_state: per-open state management
 *   - zfs_max_nvlist_src_size_os: ioctl size limit
 *   - zfs_ioctl_update_mount_cache: mount cache update
 *   - zfs_ioctl_init_os: OS-specific ioctl registration
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/nvpair.h>
#include <sys/zfs_context.h>
#include <sys/spa_impl.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_ioctl_impl.h>
#include <sys/zfs_onexit.h>
#include <sys/zone.h>
#include <sys/sunddi.h>

extern pgcnt_t		physmem;

/*
 * VFS reference counting.
 *
 * On illumos, getzfsvfs() returns a zfsvfs with the VFS held via VFS_HOLD.
 * zfs_vfs_ref() verifies a zfsvfs is still mounted by checking z_vfs and
 * taking an additional VFS_HOLD.  zfs_vfs_rele() drops it with VFS_RELE.
 */
int
zfs_vfs_ref(zfsvfs_t **zfvp)
{
	if (*zfvp == NULL || (*zfvp)->z_vfs == NULL) {
		*zfvp = NULL;
		return (SET_ERROR(ESRCH));
	}

	VFS_HOLD((*zfvp)->z_vfs);
	return (0);
}

boolean_t
zfs_vfs_held(zfsvfs_t *zfsvfs)
{
	return (zfsvfs->z_vfs != NULL);
}

void
zfs_vfs_rele(zfsvfs_t *zfsvfs)
{
	VFS_RELE(zfsvfs->z_vfs);
}

/*
 * /dev/zfs device open.
 *
 * Minor 0 is the control device used for ioctls.  Each open of the control
 * device allocates a new minor and associated state for onexit/zevent
 * tracking.  Non-zero minors are zvol devices (handled separately once
 * zvols are wired up).
 */
int
zfsdev_open(dev_t *devp, int flag, int otyp, cred_t *cr)
{
	int error;

	(void) flag, (void) otyp, (void) cr;

	if (getminor(*devp) != 0) {
		/* zvol opens will be routed here in future */
		return (SET_ERROR(ENXIO));
	}

	/* Control device open: allocate per-fd state */
	mutex_enter(&zfsdev_state_lock);
	error = zfsdev_state_init(devp);
	mutex_exit(&zfsdev_state_lock);

	return (error);
}

/*
 * /dev/zfs device close.
 *
 * For the control device (state was allocated in open), destroy the
 * onexit and zevent state.  Minor 0 with no exclusive state is a
 * simple close with nothing to do.
 */
int
zfsdev_close(dev_t dev, int flag, int otyp, cred_t *cr)
{
	(void) flag, (void) otyp, (void) cr;

	if (getminor(dev) == 0) {
		/* Shared opens of minor 0 have no per-fd state */
		return (0);
	}

	zfsdev_state_destroy(&dev);

	return (0);
}

/*
 * /dev/zfs ioctl.
 *
 * All ZFS ioctls arrive here.  We copyin the zfs_cmd_t, dispatch to
 * the common ioctl handler, then copyout the result.
 */
int
zfsdev_ioctl(dev_t dev, int cmd, intptr_t arg, int flag, cred_t *cr,
    int *rvalp)
{
	zfs_cmd_t *zc;
	uint_t vecnum;
	int error, rc;

	(void) cr, (void) rvalp;

	vecnum = cmd - ZFS_IOC_FIRST;

	zc = kmem_zalloc(sizeof (zfs_cmd_t), KM_SLEEP);

	if (ddi_copyin((void *)(uintptr_t)arg, zc,
	    sizeof (zfs_cmd_t), flag)) {
		kmem_free(zc, sizeof (zfs_cmd_t));
		return (SET_ERROR(EFAULT));
	}

	error = zfsdev_ioctl_common(vecnum, zc, flag & FKIOCTL);

	rc = ddi_copyout(zc, (void *)(uintptr_t)arg,
	    sizeof (zfs_cmd_t), flag);
	if (error == 0 && rc != 0)
		error = SET_ERROR(EFAULT);

	kmem_free(zc, sizeof (zfs_cmd_t));

	return (error);
}

/*
 * Device attach/detach.
 *
 * On illumos, actual device node creation is handled by DDI attach in
 * kmod_core.c.  These hooks exist for the platform-independent
 * zfs_kmod_init/fini framework.
 */
int
zfsdev_attach(void)
{
	return (0);
}

void
zfsdev_detach(void)
{
}

/*
 * Per-open private state management.
 *
 * On illumos the dev_t itself is used to look up the minor, so we store
 * the zfsdev_state_t pointer keyed by the minor number that was assigned
 * in zfsdev_state_init().  The "priv" argument on illumos is a pointer
 * to the dev_t (for open) or a pointer holding the dev_t value (for
 * close/destroy).
 */
void
zfsdev_private_set_state(void *priv, zfsdev_state_t *zs)
{
	dev_t *devp;

	if (priv == NULL || zs == NULL)
		return;

	devp = priv;
	*devp = makedevice(getmajor(*devp), zs->zs_minor);
}

zfsdev_state_t *
zfsdev_private_get_state(void *priv)
{
	dev_t *devp;
	minor_t minor;

	if (priv == NULL)
		return (NULL);

	devp = priv;
	minor = getminor(*devp);
	return (zfsdev_get_state(minor, ZST_ALL));
}

/*
 * Maximum nvlist source size.
 *
 * Limits the size of ioctl input nvlists to avoid excessive memory
 * consumption.  On illumos we use a quarter of physical memory with
 * a 128 MB cap, matching the Linux behaviour.
 */
uint64_t
zfs_max_nvlist_src_size_os(void)
{
	if (zfs_max_nvlist_src_size != 0)
		return (zfs_max_nvlist_src_size);

	return (MIN(ptob(physmem) / 4, 128 * 1024 * 1024));
}

/*
 * Update the VFS mount cache after a property change.
 *
 * On illumos this is currently a no-op.  The mount table is maintained
 * by the VFS layer and updated by the generic property-change path.
 */
void
zfs_ioctl_update_mount_cache(const char *dsname)
{
	(void) dsname;
}

/*
 * Register OS-specific ioctls.
 *
 * On illumos, zone delegation (jail/unjail equivalent) will be
 * registered here once the zone_dataset_attach/detach interfaces
 * are wired up.  For now, nothing to do.
 */
void
zfs_ioctl_init_os(void)
{
}
