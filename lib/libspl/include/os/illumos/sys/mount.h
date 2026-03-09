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

#ifndef _LIBSPL_ILLUMOS_SYS_MOUNT_H
#define	_LIBSPL_ILLUMOS_SYS_MOUNT_H

/*
 * illumos has native mount(2)/umount2(2) and MS_* flags.
 */
#include_next <sys/mount.h>

/*
 * MS_OVERLAY allows mounting over a non-empty directory.
 * Define if not already present.
 */
#ifndef MS_OVERLAY
#define	MS_OVERLAY	0x00000004
#endif

/*
 * MS_CRYPT indicates that encryption keys should be loaded.
 */
#ifndef MS_CRYPT
#define	MS_CRYPT	0x00000008
#endif

#endif /* _LIBSPL_ILLUMOS_SYS_MOUNT_H */
