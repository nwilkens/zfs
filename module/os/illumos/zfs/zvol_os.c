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
 * Copyright (c) 2012, 2017 by Delphix. All rights reserved.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ZFS volume (zvol) OS-specific routines for illumos.
 *
 * These are stubs: zvol_os_create_minor() returns ENOTSUP because the
 * full illumos zvol block-device driver (minor node creation, DKIOC
 * ioctls, strategy entry point, etc.) has not yet been ported.  Since
 * create_minor fails, no zvol_state_t will ever be allocated, so the
 * remaining functions are safe no-ops.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/zfs_context.h>
#include <sys/dmu.h>
#include <sys/spa.h>
#include <sys/zap.h>
#include <sys/dmu_objset.h>
#include <sys/zfs_rlock.h>
#include <sys/dataset_kstats.h>
#include <sys/zvol.h>
#include <sys/zvol_impl.h>
#include <sys/sunddi.h>

/*
 * Free a zvol_state_t and all associated resources.
 *
 * This is the platform-specific counterpart to the common zvol teardown.
 * On illumos, zvol_os_create_minor() currently returns ENOTSUP, so this
 * should never be called.  When zvols are fully wired up, this will need
 * to destroy the DDI minor node and any platform-specific state in zv_zso.
 */
void
zvol_os_free(zvol_state_t *zv)
{
	ASSERT(!RW_LOCK_HELD(&zv->zv_suspend_lock));
	ASSERT(!MUTEX_HELD(&zv->zv_state_lock));
	ASSERT0(zv->zv_open_count);

	rw_destroy(&zv->zv_suspend_lock);
	zfs_rangelock_fini(&zv->zv_rangelock);

	cv_destroy(&zv->zv_removing_cv);
	mutex_destroy(&zv->zv_state_lock);
	dataset_kstats_destroy(&zv->zv_kstat);

	kmem_free(zv, sizeof (zvol_state_t));
}

/*
 * Create a minor node for the specified volume.
 *
 * Not yet implemented on illumos.  This is the main entry point that
 * needs DDI minor-node creation, zvol soft-state allocation, and
 * registration of the block/char device.
 */
int
zvol_os_create_minor(const char *name)
{
	(void) name;
	return (SET_ERROR(ENOTSUP));
}

/*
 * Rename a zvol minor node.
 */
int
zvol_os_rename_minor(zvol_state_t *zv, const char *newname)
{
	(void) zv;
	(void) newname;
	return (SET_ERROR(ENOTSUP));
}

/*
 * Update the volume size after a "zfs set volsize" operation.
 */
int
zvol_os_update_volsize(zvol_state_t *zv, uint64_t volsize)
{
	(void) zv;
	(void) volsize;
	return (SET_ERROR(ENOTSUP));
}

/*
 * Check whether a given path refers to a zvol device.
 *
 * On illumos, zvol device paths live under /dev/zvol/dsk and
 * /dev/zvol/rdsk.  Since zvols are not yet supported, always
 * return B_FALSE.
 */
boolean_t
zvol_os_is_zvol(const char *path)
{
	(void) path;
	return (B_FALSE);
}

/*
 * Remove a zvol minor node.
 */
void
zvol_os_remove_minor(zvol_state_t *zv)
{
	(void) zv;
}

/*
 * Set a zvol device to read-only mode.
 */
void
zvol_os_set_disk_ro(zvol_state_t *zv, int flags)
{
	(void) zv;
	(void) flags;
}

/*
 * Notify the OS that the zvol capacity has changed.
 */
void
zvol_os_set_capacity(zvol_state_t *zv, uint64_t capacity)
{
	(void) zv;
	(void) capacity;
}

/*
 * Wait for all open references to close on the given zvol.
 *
 * Since zvols are not yet functional on illumos (create_minor returns
 * ENOTSUP), this is a no-op.
 */
void
zvol_wait_close(zvol_state_t *zv)
{
	(void) zv;
}

/*
 * Initialize the zvol subsystem.
 * Calls the common zvol_init_impl() which sets up the zvol hash table
 * and registers with the SPA.
 */
int
zvol_init(void)
{
	return (zvol_init_impl());
}

/*
 * Tear down the zvol subsystem.
 */
void
zvol_fini(void)
{
	zvol_fini_impl();
}
