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

#ifndef _LIBSPL_ILLUMOS_SYS_STAT_H
#define	_LIBSPL_ILLUMOS_SYS_STAT_H

#include_next <sys/stat.h>

/* Forward-declare ioctl to avoid implicit declaration issues */
extern int ioctl(int, int, ...);

#include <sys/dkio.h>

#ifndef MAXOFFSET_T
#define	MAXOFFSET_T	(0x7fffffffffffffffLL)
#endif

/*
 * Emulate the fstat64_blk() behavior: for block devices, use an ioctl
 * to fill in st_size with the device size.
 */
static inline int
fstat64_blk(int fd, struct stat64 *st)
{
	if (fstat64(fd, st) == -1)
		return (-1);

	if (S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode)) {
		struct dk_minfo dkm;
		if (ioctl(fd, DKIOCGMEDIAINFO, &dkm) == 0) {
			st->st_size = (off64_t)dkm.dki_capacity *
			    (off64_t)dkm.dki_lbsize;
		}
	}

	return (0);
}

#endif /* _LIBSPL_ILLUMOS_SYS_STAT_H */
