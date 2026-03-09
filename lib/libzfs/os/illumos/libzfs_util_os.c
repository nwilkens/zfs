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
 * illumos-specific utility functions for libzfs.
 */

#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>

#include <libzfs.h>
#include <libzfs_core.h>

#include "../../libzfs_impl.h"
#include <libzutil.h>

#define	ZDIFF_SHARESDIR		"/.zfs/shares/"

const char *
libzfs_error_init(int error)
{
	switch (error) {
	case ENXIO:
		return (dgettext(TEXT_DOMAIN, "The ZFS modules are not "
		    "loaded.\nTry running 'modload misc/zfs' as root "
		    "to load them."));
	case ENOENT:
		return (dgettext(TEXT_DOMAIN, "/dev/zfs is required.\n"
		    "Verify that the ZFS device node exists."));
	case ENOEXEC:
		return (dgettext(TEXT_DOMAIN, "The ZFS modules cannot be "
		    "auto-loaded.\nTry running 'modload misc/zfs' as "
		    "root to manually load them."));
	case EACCES:
		return (dgettext(TEXT_DOMAIN, "Permission denied the "
		    "ZFS utilities must be run as root."));
	default:
		return (dgettext(TEXT_DOMAIN, "Failed to initialize the "
		    "libzfs library."));
	}
}

/*
 * Verify the required ZFS_DEV device is available and optionally attempt
 * to load the ZFS modules.
 *
 * On illumos, the ZFS module is typically already loaded as part of the
 * base system.  We just check for the /dev/zfs device node.
 */
int
libzfs_load_module(void)
{
	if (access(ZFS_DEV, F_OK) == 0)
		return (0);

	/*
	 * On illumos, try modctl to load the module.
	 * For now, just return ENOENT if /dev/zfs doesn't exist.
	 */
	return (ENOENT);
}

int
find_shares_object(differ_info_t *di)
{
	char fullpath[MAXPATHLEN];
	struct stat64 sb = { 0 };

	(void) strlcpy(fullpath, di->dsmnt, MAXPATHLEN);
	(void) strlcat(fullpath, ZDIFF_SHARESDIR, MAXPATHLEN);

	if (stat64(fullpath, &sb) != 0) {
		(void) snprintf(di->errbuf, sizeof (di->errbuf),
		    dgettext(TEXT_DOMAIN, "Cannot stat %s"), fullpath);
		return (zfs_error(di->zhp->zfs_hdl, EZFS_DIFF, di->errbuf));
	}

	di->shares = (uint64_t)sb.st_ino;
	return (0);
}

int
zfs_destroy_snaps_nvl_os(libzfs_handle_t *hdl, nvlist_t *snaps)
{
	(void) hdl, (void) snaps;
	return (0);
}

/*
 * Return allocated loaded module version, or NULL on error (with errno set).
 *
 * On illumos, we read the module version from /dev/zfs via an ioctl,
 * or from the SPL parameters.  For now, return the userland version
 * as a placeholder until the kernel module is available.
 */
char *
zfs_version_kernel(void)
{
	/*
	 * If the ZFS device node exists, we could query the kernel for
	 * its version.  Until then, return NULL with errno set to indicate
	 * the module is not loaded.
	 */
	if (access(ZFS_DEV, F_OK) != 0) {
		errno = ENOENT;
		return (NULL);
	}

	/*
	 * Placeholder: return a copy of the userland version string.
	 * When the kernel module is available, this should query it
	 * via ioctl.
	 */
	return (strdup(ZFS_META_VERSION));
}
