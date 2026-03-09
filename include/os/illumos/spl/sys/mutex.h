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

#ifndef _SPL_SYS_MUTEX_H
#define	_SPL_SYS_MUTEX_H

/*
 * illumos provides kmutex_t, kmutex_type_t (MUTEX_DEFAULT, MUTEX_SPIN,
 * MUTEX_ADAPTIVE, MUTEX_DRIVER), mutex_init, mutex_destroy, mutex_enter,
 * mutex_tryenter, mutex_exit, mutex_owned, mutex_owner natively.
 *
 * Chain to the real header.
 */
#include_next <sys/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MUTEX_HELD / MUTEX_NOT_HELD — convenience predicates used throughout
 * OpenZFS.  illumos provides mutex_owned() which does the same thing.
 */
#ifndef MUTEX_HELD
#define	MUTEX_HELD(x)		(mutex_owned(x))
#endif

#ifndef MUTEX_NOT_HELD
#define	MUTEX_NOT_HELD(x)	(!mutex_owned(x))
#endif

/*
 * mutex_enter_interruptible — on illumos, mutex_enter is already
 * interruptible in the sense that it will not deadlock; there is no
 * separate signal-interruptible variant in the kernel.  Map to
 * mutex_enter() and return 0 (success) for API compatibility.
 *
 * Callers that need true signal-interruptible behaviour must use
 * a condvar or implement their own check.
 */
#ifndef mutex_enter_interruptible
static inline int
mutex_enter_interruptible(kmutex_t *mp)
{
	mutex_enter(mp);
	return (0);
}
#endif

/*
 * mutex_enter_nested — illumos has no nested-lock variant; map to
 * mutex_enter().  The subclass argument is ignored.
 */
#ifndef mutex_enter_nested
#define	mutex_enter_nested(mp, subclass)	mutex_enter(mp)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_MUTEX_H */
