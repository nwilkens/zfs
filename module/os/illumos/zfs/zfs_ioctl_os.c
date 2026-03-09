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
 * /dev/zfs device creation and ioctl dispatch.
 * Stubs for now -- will be filled in with illumos DDI device code.
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
#include <sys/zfs_vfsops.h>
#include <sys/zone.h>
#include <sys/sunddi.h>

int
zfs_vfs_ref(zfsvfs_t **zfvp)
{
	int error = 0;

	if (*zfvp == NULL)
		return (SET_ERROR(ESRCH));

	error = vfs_lock((*zfvp)->z_vfs);
	if (error != 0) {
		*zfvp = NULL;
		error = SET_ERROR(ESRCH);
	} else {
		vfs_unlock((*zfvp)->z_vfs);
	}
	return (error);
}

boolean_t
zfs_vfs_held(zfsvfs_t *zfsvfs)
{
	return (zfsvfs->z_vfs != NULL);
}

int
zfsdev_attach(void)
{
	/* Device creation handled by DDI attach in kmod_core.c */
	return (0);
}

void
zfsdev_detach(void)
{
	/* Device destruction handled by DDI detach in kmod_core.c */
}

void
zfsdev_private_set_state(void *priv, zfsdev_state_t *zs)
{
	(void) priv;
	(void) zs;
	/* TODO: implement using minor node private data */
}

zfsdev_state_t *
zfsdev_private_get_state(void *priv)
{
	(void) priv;
	/* TODO: implement using minor node private data */
	return (NULL);
}
