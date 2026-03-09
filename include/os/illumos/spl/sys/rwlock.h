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

#ifndef _SPL_SYS_RWLOCK_H
#define	_SPL_SYS_RWLOCK_H

/*
 * illumos provides krwlock_t, krw_type_t (RW_DEFAULT, RW_DRIVER),
 * krw_t (RW_READER, RW_WRITER, RW_NONE),
 * rw_init, rw_destroy, rw_enter, rw_exit, rw_tryenter, rw_downgrade,
 * rw_tryupgrade, rw_read_held, rw_write_held, rw_lock_held, rw_iswriter,
 * rw_owner natively.
 *
 * Chain to the real header.
 */
#include_next <sys/rwlock.h>

/*
 * RW_NONE — not defined on illumos (krw_t only has RW_READER, RW_WRITER).
 * OpenZFS uses RW_NONE = 0 as a "no lock held" sentinel value.
 */
#ifndef RW_NONE
#define	RW_NONE		0
#endif

#endif	/* _SPL_SYS_RWLOCK_H */
