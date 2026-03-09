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

#ifndef _SPL_SYS_SYSEVENT_H
#define	_SPL_SYS_SYSEVENT_H

/*
 * illumos has native sysevent infrastructure (sysevent_t, sysevent_id_t,
 * log_sysevent, etc.).  OpenZFS has a simplified stub in
 * include/sys/sysevent.h that shares the same include guard
 * (_SYS_SYSEVENT_H) but lacks sysevent_id_t.
 *
 * When #include_next chains through, it hits the OpenZFS stub before
 * the real system header.  We must ensure the real illumos header's
 * definitions are available.  We include the types directly.
 */
#include <sys/nvpair.h>
#include <sys/types.h>
#include <sys/time.h>

/*
 * sysevent_t — for the resource interface used by OpenZFS
 */
#ifndef _SYS_SYSEVENT_H
typedef struct sysevent {
	nvlist_t *resource;
} sysevent_t;
#endif

/*
 * sysevent_id_t — unique event identifier, needed by system sunddi.h.
 * On illumos this is defined in /usr/include/sys/sysevent.h.
 */
typedef struct sysevent_id {
	uint64_t eid_seq;
	hrtime_t eid_ts;
} sysevent_id_t;

/*
 * Ensure _SYS_SYSEVENT_H is set so both the OpenZFS stub and the
 * system header are no-ops if included later.
 */
#define	_SYS_SYSEVENT_H

#endif	/* _SPL_SYS_SYSEVENT_H */
