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

#ifndef _SPL_SYS_TASKQ_H
#define	_SPL_SYS_TASKQ_H

/*
 * illumos provides taskq_t, taskqid_t, task_func_t, taskq_ent_t,
 * taskq_create, taskq_create_instance, taskq_create_proc, taskq_create_sysdc,
 * taskq_destroy, taskq_dispatch, taskq_dispatch_delay, taskq_dispatch_ent,
 * taskq_wait, taskq_wait_id, taskq_wait_outstanding, taskq_cancel_id,
 * taskq_member, taskq_of_curthread, taskq_suspend, taskq_suspended,
 * taskq_resume, taskq_init_ent, taskq_empty_ent, nulltask,
 * system_taskq, system_delay_taskq, TASKQID_INVALID,
 * TASKQ_PREPOPULATE, TASKQ_CPR_SAFE, TASKQ_DYNAMIC, TASKQ_THREADS_CPU_PCT,
 * TASKQ_DC_BATCH, TQ_SLEEP, TQ_NOSLEEP, TQ_NOQUEUE, TQ_NOALLOC, TQ_FRONT
 * natively.
 *
 * Chain to the real header.
 */
#include_next <sys/taskq.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * system_delay_taskq — global delayed-dispatch task queue.
 * illumos does not provide this natively; the SPL module creates it.
 */
extern taskq_t *system_delay_taskq;

/*
 * taskq_dispatch_delay — dispatch a task with a delay.
 * illumos does not provide this natively; the SPL module implements it.
 */
extern taskqid_t taskq_dispatch_delay(taskq_t *, task_func_t, void *,
    uint_t, clock_t);

/*
 * taskq_init_ent — initialize a taskq_ent_t for use with taskq_dispatch_ent.
 * illumos does not expose this function; define as a no-op.
 */
#ifndef taskq_init_ent
#define	taskq_init_ent(x)
#endif

/*
 * taskq_create_synced — creates a taskq and returns the array of backing
 * kthreads via the third argument.  illumos does not expose this variant
 * natively; declare it for OpenZFS code that references it.  The illumos
 * port must provide an implementation.
 */
#ifndef taskq_create_synced
extern taskq_t *taskq_create_synced(const char *name, int nthreads,
    pri_t pri, int minalloc, int maxalloc, uint_t flags,
    kthread_t ***ktpp);
#endif

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_TASKQ_H */
