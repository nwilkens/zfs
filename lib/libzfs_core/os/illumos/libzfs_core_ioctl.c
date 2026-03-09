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
 * CDDL HEADER END
 */
#include <sys/types.h>
#include <sys/zfs_ioctl.h>
#include <libzfs_core.h>
#include "libzfs_core_impl.h"

int
lzc_ioctl_fd_os(int fd, unsigned long request, zfs_cmd_t *zc)
{
	return (ioctl(fd, request, zc));
}
