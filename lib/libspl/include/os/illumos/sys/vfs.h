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

#ifndef _LIBSPL_ILLUMOS_SYS_VFS_H
#define	_LIBSPL_ILLUMOS_SYS_VFS_H

#include <sys/statvfs.h>

int fsshare(const char *, const char *, const char *);
int fsunshare(const char *, const char *);

#endif /* _LIBSPL_ILLUMOS_SYS_VFS_H */
