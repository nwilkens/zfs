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
 * Copyright (c) 2024, OpenZFS Contributors.  All rights reserved.
 */

#ifndef _SPL_SYS_DEBUG_H
#define	_SPL_SYS_DEBUG_H

/*
 * illumos already provides ASSERT, VERIFY, ASSERT3U, ASSERT3S, ASSERT3P,
 * ASSERT0, VERIFY3U, VERIFY3S, VERIFY3P, VERIFY0 in <sys/debug.h>.
 * Chain to the real header.
 */
#include_next <sys/debug.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * __must_check — mark return values that must be checked.
 */
#ifndef __must_check
#define	__must_check __attribute__((__warn_unused_result__))
#endif

/*
 * __printflike — used by OpenZFS for printf format checking.
 * FreeBSD defines it in <sys/cdefs.h>; Linux maps to __printf.
 * On illumos, use the GCC attribute directly.
 */
#ifndef __printflike
#define	__printflike(fmtarg, firstvararg) \
	__attribute__((__format__(__printf__, fmtarg, firstvararg)))
#endif

/*
 * ASSERT0P / VERIFY0P — assert a pointer is NULL.
 * illumos does not define these; add them here.
 */
#ifndef ASSERT0P
#define	ASSERT0P(x)	ASSERT3P(x, ==, NULL)
#endif

#ifndef VERIFY0P
#define	VERIFY0P(x)	VERIFY3P(x, ==, NULL)
#endif

/*
 * IMPLY(A, B) — verify that A implies B (i.e., !A || B).
 * EQUIV(A, B) — verify that A is logically equivalent to B.
 * illumos may not define these; provide fallbacks.
 */
#ifndef IMPLY
#define	IMPLY(A, B)	\
	((void)(((!(A)) || (B)) || \
	    (panic("(" #A ") implies (" #B ")"), 0)))
#endif

#ifndef EQUIV
#define	EQUIV(A, B)	ASSERT3B(!!(A), ==, !!(B))
#endif

/*
 * ASSERTF / VERIFYF — assert/verify with a format string.
 * illumos may not define these; provide fallbacks.
 */
#ifndef ASSERTF
#define	ASSERTF(x, fmt, ...)	ASSERT(x)
#endif

#ifndef VERIFYF
#define	VERIFYF(cond, fmt, ...)	do {					\
	if (!(cond))							\
		panic("VERIFY(" #cond ") failed: " fmt, ##__VA_ARGS__); \
} while (0)
#endif

/*
 * VERIFY3SF / VERIFY3UF / VERIFY3PF / VERIFY3BF — verify with format string.
 * ASSERT3SF / ASSERT3UF / ASSERT3PF / ASSERT3BF — debug-only variants.
 * ASSERT0PF / ASSERT0F — zero-check with format string.
 * Not present in illumos; add them here following the FreeBSD/Linux pattern.
 */
#ifndef VERIFY3SF
#define	VERIFY3SF(x, y, z, str, ...)	VERIFY3S(x, y, z)
#endif
#ifndef VERIFY3UF
#define	VERIFY3UF(x, y, z, str, ...)	VERIFY3U(x, y, z)
#endif
#ifndef VERIFY3PF
#define	VERIFY3PF(x, y, z, str, ...)	VERIFY3P(x, y, z)
#endif
#ifndef VERIFY3BF
#define	VERIFY3BF(x, y, z, str, ...)	VERIFY3B(x, y, z)
#endif
#ifndef VERIFY0PF
#define	VERIFY0PF(x, str, ...)		VERIFY0P(x)
#endif
#ifndef VERIFY0F
#define	VERIFY0F(x, str, ...)		VERIFY0(x)
#endif

#ifdef ZFS_DEBUG
#define	ASSERT3SF	VERIFY3SF
#define	ASSERT3UF	VERIFY3UF
#define	ASSERT3PF	VERIFY3PF
#define	ASSERT3BF	VERIFY3BF
#define	ASSERT0PF	VERIFY0PF
#define	ASSERT0F	VERIFY0F
#else
#define	ASSERT3SF(x, y, z, str, ...)	((void)0)
#define	ASSERT3UF(x, y, z, str, ...)	((void)0)
#define	ASSERT3PF(x, y, z, str, ...)	((void)0)
#define	ASSERT3BF(x, y, z, str, ...)	((void)0)
#define	ASSERT0PF(x, str, ...)		((void)0)
#define	ASSERT0F(x, str, ...)		((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_DEBUG_H */
