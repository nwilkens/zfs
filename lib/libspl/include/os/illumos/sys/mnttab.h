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

#ifndef _LIBSPL_ILLUMOS_SYS_MNTTAB_H
#define	_LIBSPL_ILLUMOS_SYS_MNTTAB_H

/*
 * illumos natively provides <sys/mnttab.h> with struct mnttab,
 * struct extmnttab, getmntany(), getmntent(), hasmntopt(), etc.
 *
 * However, the native getextmntent() has a different signature than
 * what OpenZFS expects:
 *   native:  int getextmntent(FILE *, struct extmnttab *, size_t)
 *   OpenZFS: int getextmntent(const char *path, struct extmnttab *,
 *                              struct stat64 *)
 *
 * We rename the native one to avoid the conflict.
 */
#define	getextmntent	native_getextmntent
#include_next <sys/mnttab.h>
#undef	getextmntent

#include <sys/types.h>

struct stat64;

extern int getextmntent(const char *path, struct extmnttab *entry,
    struct stat64 *statbuf);

#endif /* _LIBSPL_ILLUMOS_SYS_MNTTAB_H */
