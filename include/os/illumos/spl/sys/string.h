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

#ifndef _SPL_SYS_STRING_H
#define	_SPL_SYS_STRING_H

/*
 * illumos does not have <sys/string.h>. Kernel string functions
 * (bcopy, bzero, strlen, strcmp, etc.) live in <sys/systm.h>.
 * Userland uses <string.h>.
 */
#ifdef _KERNEL
#include <sys/systm.h>
#else
#include <string.h>
#endif

#endif	/* _SPL_SYS_STRING_H */
