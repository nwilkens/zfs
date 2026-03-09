// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

#ifndef _SPL_SYS_ISA_DEFS_H
#define	_SPL_SYS_ISA_DEFS_H

/*
 * illumos has native <sys/isa_defs.h> which defines _BIG_ENDIAN/_LITTLE_ENDIAN,
 * _LP64, _ILP32, _SUNOS_VTOC_8/16, etc.
 */
#include_next <sys/isa_defs.h>

/*
 * OpenZFS uses _ZFS_BIG_ENDIAN / _ZFS_LITTLE_ENDIAN.
 * Map from the illumos native _BIG_ENDIAN / _LITTLE_ENDIAN.
 */
#if defined(_BIG_ENDIAN) && !defined(_ZFS_BIG_ENDIAN)
#define	_ZFS_BIG_ENDIAN
#endif
#if defined(_LITTLE_ENDIAN) && !defined(_ZFS_LITTLE_ENDIAN)
#define	_ZFS_LITTLE_ENDIAN
#endif

#endif	/* _SPL_SYS_ISA_DEFS_H */
