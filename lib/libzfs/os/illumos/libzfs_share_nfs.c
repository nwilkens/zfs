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
 * illumos-specific NFS sharing support for libzfs.
 *
 * On illumos, NFS sharing is managed through sharemgr(1M) or the
 * share/unshare commands with the native NFS server.  This provides
 * the libshare_nfs_type operations for the OpenZFS sharing framework.
 *
 * For now, this is a minimal stub implementation.  When the kernel
 * module is fully ported, this should be enhanced to use the native
 * illumos sharing infrastructure (sharemgr/libshare).
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libzfs.h>
#include "../../libzfs_share.h"

/*
 * Check whether NFS sharing is available on this system.
 * On illumos, we check for the share command.
 */
static boolean_t
nfs_available(void)
{
	static int avail;

	if (!avail) {
		if (access("/usr/sbin/share", F_OK) != 0 &&
		    access("/usr/sbin/zfs", F_OK) != 0)
			avail = -1;
		else
			avail = 1;
	}

	return (avail == 1);
}

/*
 * Enables NFS sharing for the specified share.
 */
static int
nfs_enable_share(sa_share_impl_t impl_share)
{
	if (!nfs_available())
		return (SA_SYSTEM_ERR);

	const char *shareopts = impl_share->sa_shareopts;
	if (shareopts == NULL || strcmp(shareopts, "off") == 0)
		return (SA_OK);

	/*
	 * On illumos, use the share(1M) command to export via NFS.
	 * share -F nfs -o <opts> <path>
	 */
	const char *opts = (strcmp(shareopts, "on") == 0) ? "rw" : shareopts;

	char *argv[] = {
		(char *)"/usr/sbin/share",
		(char *)"-F",
		(char *)"nfs",
		(char *)"-o",
		(char *)opts,
		(char *)impl_share->sa_mountpoint,
		NULL,
	};

	if (libzfs_run_process(argv[0], argv, 0) != 0)
		return (SA_SYSTEM_ERR);

	return (SA_OK);
}

/*
 * Disables NFS sharing for the specified share.
 */
static int
nfs_disable_share(sa_share_impl_t impl_share)
{
	if (!nfs_available())
		return (SA_OK);

	char *argv[] = {
		(char *)"/usr/sbin/unshare",
		(char *)"-F",
		(char *)"nfs",
		(char *)impl_share->sa_mountpoint,
		NULL,
	};

	(void) libzfs_run_process(argv[0], argv, 0);

	return (SA_OK);
}

/*
 * Checks whether the specified share is currently active.
 */
static boolean_t
nfs_is_shared(sa_share_impl_t impl_share)
{
	(void) impl_share;
	/*
	 * TODO: Parse /etc/dfs/sharetab to determine if the
	 * mountpoint is currently shared.
	 */
	return (B_FALSE);
}

/*
 * Checks whether the specified NFS share options are syntactically correct.
 */
static int
nfs_validate_shareopts(const char *shareopts)
{
	if (shareopts == NULL || strlen(shareopts) == 0)
		return (SA_SYNTAX_ERR);

	return (SA_OK);
}

static int
nfs_commit_shares(void)
{
	/* On illumos, shares take effect immediately via share(1M). */
	return (SA_OK);
}

static void
nfs_truncate_shares(void)
{
	/* Nothing to do on illumos. */
}

const sa_fstype_t libshare_nfs_type = {
	.enable_share = nfs_enable_share,
	.disable_share = nfs_disable_share,
	.is_shared = nfs_is_shared,

	.validate_shareopts = nfs_validate_shareopts,
	.commit_shares = nfs_commit_shares,
	.truncate_shares = nfs_truncate_shares,
};
