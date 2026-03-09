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

#ifndef _SPL_SYS_CONDVAR_H
#define	_SPL_SYS_CONDVAR_H

/*
 * illumos provides kcondvar_t, kcv_type_t (CV_DEFAULT, CV_DRIVER),
 * cv_init, cv_destroy, cv_wait, cv_wait_sig, cv_timedwait,
 * cv_timedwait_sig, cv_timedwait_hires, cv_timedwait_sig_hires,
 * cv_signal, cv_broadcast natively.
 *
 * Chain to the real header.
 */
#include_next <sys/condvar.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * IO/idle variants — on illumos these are functionally identical to the
 * standard variants.  Map them to the native functions.
 */
#ifndef cv_wait_io
#define	cv_wait_io(cvp, mp)		cv_wait(cvp, mp)
#endif

#ifndef cv_wait_io_sig
#define	cv_wait_io_sig(cvp, mp)		cv_wait_sig(cvp, mp)
#endif

#ifndef cv_wait_idle
#define	cv_wait_idle(cvp, mp)		cv_wait(cvp, mp)
#endif

#ifndef cv_timedwait_io
#define	cv_timedwait_io(cvp, mp, t)	cv_timedwait(cvp, mp, t)
#endif

#ifndef cv_timedwait_idle
#define	cv_timedwait_idle(cvp, mp, t)	cv_timedwait(cvp, mp, t)
#endif

#ifndef cv_timedwait_sig_io
#define	cv_timedwait_sig_io(cvp, mp, t)	cv_timedwait_sig(cvp, mp, t)
#endif

/*
 * cv_timedwait_io_hires / cv_timedwait_idle_hires — high-resolution
 * IO/idle variants.  illumos cv_timedwait_hires already handles these.
 */
#ifndef cv_timedwait_io_hires
#define	cv_timedwait_io_hires(cvp, mp, t, r, f)	\
	cv_timedwait_hires(cvp, mp, t, r, f)
#endif

#ifndef cv_timedwait_idle_hires
#define	cv_timedwait_idle_hires(cvp, mp, t, r, f)	\
	cv_timedwait_hires(cvp, mp, t, r, f)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_CONDVAR_H */
