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

/*
 * On illumos, getmntany(), getmntent(), and hasmntopt() are all provided
 * natively by libc via <sys/mnttab.h>.  This file only provides the
 * OpenZFS-specific getextmntent() which takes a path and stat64 buffer
 * (unlike the native getextmntent which takes a FILE* and size).
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/mnttab.h>
#include <sys/param.h>
#include <libzutil.h>

/*
 * The native illumos getextmntent is renamed to native_getextmntent by
 * our mnttab.h header (to avoid signature conflict with the OpenZFS version).
 * Use asm label to map the renamed declaration to the real libc symbol.
 */
extern int native_getextmntent(FILE *, struct extmnttab *, size_t)
    __asm__("getextmntent");

int
getextmntent(const char *path, struct extmnttab *entry, struct stat64 *statbuf)
{
	FILE *fp;
	struct extmnttab ent;

	if (strlen(path) >= MAXPATHLEN) {
		(void) fprintf(stderr, "invalid object; pathname too long\n");
		return (-1);
	}

	if (stat64(path, statbuf) != 0) {
		(void) fprintf(stderr, "cannot open '%s': %s\n",
		    path, zfs_strerror(errno));
		return (-1);
	}

	fp = fopen(MNTTAB, "r");
	if (fp == NULL)
		return (-1);

	resetmnttab(fp);
	while (native_getextmntent(fp, &ent, sizeof (ent)) == 0) {
		if (makedev(ent.mnt_major, ent.mnt_minor) ==
		    statbuf->st_dev) {
			*entry = ent;
			(void) fclose(fp);
			return (0);
		}
	}

	(void) fclose(fp);
	return (-1);
}
