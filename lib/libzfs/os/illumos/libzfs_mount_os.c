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
 * Copyright 2025 Oxide Computer Company
 */

/*
 * illumos-specific mount/unmount support for libzfs.
 *
 * On illumos, ZFS mount/unmount is handled via the native mount(2) and
 * umount2(2) system calls.  Until the OpenZFS kernel module is ported
 * to illumos, these functions return ENOTSUP as stubs.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/mntent.h>
#include <libzfs.h>

#include "../../libzfs_impl.h"

/*
 * illumos does not use the Linux-style mount option parsing.
 * Provide stubs for the functions declared in libzfs.h that are
 * only meaningful on Linux.
 */
int
zfs_parse_mount_options(const char *mntopts, unsigned long *mntflags,
    unsigned long *zfsflags, int sloppy, char *badopt, char *mtabopt)
{
	(void) mntopts, (void) mntflags, (void) zfsflags;
	(void) sloppy, (void) badopt, (void) mtabopt;
	return (0);
}

void
zfs_adjust_mount_options(zfs_handle_t *zhp, const char *mntpoint,
    char *mntopts, char *mtabopt)
{
	(void) zhp, (void) mntpoint, (void) mntopts, (void) mtabopt;
}

/*
 * Mount a ZFS filesystem.
 *
 * On illumos, this will use mount(2) with fstype "zfs".  Until the
 * OpenZFS kernel module is available, this returns ENOTSUP.
 */
int
do_mount(zfs_handle_t *zhp, const char *mntpt, const char *opts, int flags)
{
	const char *src = zfs_get_name(zhp);
	int mflag = flags;

	/*
	 * Parse mount options.  On illumos the mount(2) syscall takes
	 * a flags word and a data pointer for fs-specific options.
	 */
	if (opts != NULL && strstr(opts, "ro") != NULL)
		mflag |= MS_RDONLY;

	if (mount(src, mntpt, mflag, MNTTYPE_ZFS, NULL, 0,
	    (char *)opts, strlen(opts ? opts : "")) != 0) {
		return (errno);
	}
	return (0);
}

/*
 * Unmount a ZFS filesystem.
 *
 * On illumos, this uses umount2(2).
 */
int
do_unmount(zfs_handle_t *zhp, const char *mntpt, int flags)
{
	(void) zhp;
	int umnt_flags = 0;

	if (flags & MS_FORCE)
		umnt_flags |= MS_FORCE;

	if (umount2(mntpt, umnt_flags) != 0)
		return (errno);

	return (0);
}

int
zfs_mount_delegation_check(void)
{
	return ((geteuid() != 0) ? EACCES : 0);
}

/* Called from the tail end of zpool_disable_datasets() */
void
zpool_disable_datasets_os(zpool_handle_t *zhp, boolean_t force)
{
	(void) zhp, (void) force;
}

/* Called from the tail end of zfs_unmount() */
void
zpool_disable_volume_os(const char *name)
{
	(void) name;
}
