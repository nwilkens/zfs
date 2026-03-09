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
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

/*
 * SPL sysevent / zevent compatibility for illumos.
 *
 * illumos has native sysevent infrastructure (sysevent_t, log_sysevent(),
 * etc.).  OpenZFS uses its own zevent mechanism.  On illumos, zevents
 * are dispatched via the standard sysevent channel.
 *
 * The sysevent worker thread reads zevents and logs them using the
 * native illumos sysevent infrastructure.
 */

#include <sys/zfs_context.h>
#include <sys/nvpair.h>
#include <sys/fm/protocol.h>
#include <sys/fm/util.h>
#include <sys/sunddi.h>
#include <sys/sysevent.h>

static void
sysevent_worker(void *arg)
{
	(void) arg;
	zfs_zevent_t *ze;
	nvlist_t *event;
	uint64_t dropped = 0;
	uint64_t dst_size;
	int error;

	zfs_zevent_init(&ze);
	for (;;) {
		dst_size = 131072;
		dropped = 0;
		event = NULL;
		error = zfs_zevent_next(ze, &event, &dst_size, &dropped);
		if (error) {
			error = zfs_zevent_wait(ze);
			if (error == ESHUTDOWN)
				break;
		} else {
			VERIFY3P(event, !=, NULL);
			/*
			 * On illumos, we can log the event via the native
			 * sysevent mechanism.  For now, just free the event.
			 */
			nvlist_free(event);
		}
	}

	VERIFY0P(ze->ze_zevent);
	kmem_free(ze, sizeof (zfs_zevent_t));

	thread_exit();
}

void
ddi_sysevent_init(void)
{
	(void) thread_create(NULL, 0, sysevent_worker, NULL, 0,
	    &p0, TS_RUN, minclsyspri);
}
