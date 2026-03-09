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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2011 Nexenta Systems, Inc.  All rights reserved.
 * Copyright (c) 2012, 2018 by Delphix. All rights reserved.
 * Copyright (c) 2012, Joyent, Inc. All rights reserved.
 */

#ifndef _LIBSPL_SYS_MISC_H
#define	_LIBSPL_SYS_MISC_H

#include <sys/utsname.h>

/*
 * Hostname information
 */
typedef struct utsname	utsname_t;
#if defined(__illumos__)
/*
 * On illumos, 'utsname' is both a struct name and a global variable
 * (in kernel), so we cannot redeclare it as a function.  Provide a
 * static inline that returns a cached utsname struct.
 */
static inline utsname_t *
spl_utsname(void)
{
	static utsname_t _spl_uts;
	static int _spl_uts_init;
	if (!_spl_uts_init) {
		(void) uname(&_spl_uts);
		_spl_uts_init = 1;
	}
	return (&_spl_uts);
}
#define	utsname()	spl_utsname()
#else
extern utsname_t *utsname(void);
#endif

#endif
