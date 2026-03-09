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
 * CDDL HEADER END
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * On illumos, chain to the real system <sys/feature_tests.h> which defines
 * critical macros like _RESTRICT_KYWD, _STDC_C99, etc.
 */
#ifdef __illumos__
#include_next <sys/feature_tests.h>
#endif

#ifndef _LIBSPL_SYS_FEATURE_TESTS_H
#define	_LIBSPL_SYS_FEATURE_TESTS_H

#define	____cacheline_aligned

#if !defined(zfs_fallthrough) && !defined(_LIBCPP_VERSION)
#if defined(HAVE_IMPLICIT_FALLTHROUGH)
#define	zfs_fallthrough		__attribute__((__fallthrough__))
#else
#define	zfs_fallthrough		((void)0)
#endif
#endif

#endif
