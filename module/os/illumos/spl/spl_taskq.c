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
 * SPL taskq functions for illumos.
 *
 * illumos has native taskq_create, taskq_dispatch, taskq_wait,
 * taskq_destroy, system_taskq, etc.  We provide:
 *
 *   - system_delay_taskq initialization (not in base illumos)
 *   - taskq_dispatch_delay (not in base illumos)
 *   - taskq_create_synced (not in base illumos)
 */

#include <sys/zfs_context.h>
#include <sys/taskq.h>
#include <sys/taskq_impl.h>
#include <sys/callb.h>

/*
 * Global delayed-dispatch task queue.
 * illumos does not provide this natively; we create it during module init.
 */
taskq_t *system_delay_taskq = NULL;

/*
 * Delayed dispatch — dispatch a task that will execute after a specified
 * clock tick deadline.  The implementation uses a simple wrapper that
 * sleeps until the deadline then runs the function.
 */
typedef struct taskq_delay_arg {
	task_func_t	*tda_func;
	void		*tda_arg;
	clock_t		tda_expire;
} taskq_delay_arg_t;

static void
taskq_delay_wrapper(void *arg)
{
	taskq_delay_arg_t *tda = arg;
	clock_t now = ddi_get_lbolt();
	clock_t delta = tda->tda_expire - now;

	if (delta > 0)
		delay(delta);

	tda->tda_func(tda->tda_arg);
	kmem_free(tda, sizeof (*tda));
}

taskqid_t
taskq_dispatch_delay(taskq_t *tq, task_func_t func, void *arg,
    uint_t flags, clock_t expire_time)
{
	clock_t now = ddi_get_lbolt();

	if (expire_time <= now)
		return (taskq_dispatch(tq, func, arg, flags));

	taskq_delay_arg_t *tda = kmem_alloc(sizeof (*tda), KM_SLEEP);
	tda->tda_func = func;
	tda->tda_arg = arg;
	tda->tda_expire = expire_time;

	return (taskq_dispatch(tq, taskq_delay_wrapper, tda, flags));
}

/*
 * taskq_create_synced — create a taskq and return an array of kthread_t
 * pointers for each thread in the pool.
 */
typedef struct taskq_sync_arg {
	kthread_t	*tqa_thread;
	kcondvar_t	tqa_cv;
	kmutex_t	tqa_lock;
	int		tqa_ready;
} taskq_sync_arg_t;

static void
taskq_sync_assign(void *arg)
{
	taskq_sync_arg_t *tqa = arg;

	mutex_enter(&tqa->tqa_lock);
	tqa->tqa_thread = curthread;
	tqa->tqa_ready = 1;
	cv_signal(&tqa->tqa_cv);
	while (tqa->tqa_ready == 1)
		cv_wait(&tqa->tqa_cv, &tqa->tqa_lock);
	mutex_exit(&tqa->tqa_lock);
}

taskq_t *
taskq_create_synced(const char *name, int nthreads, pri_t pri,
    int minalloc, int maxalloc, uint_t flags, kthread_t ***ktpp)
{
	taskq_t *tq;
	taskq_sync_arg_t *tqs;
	kthread_t **kthreads;
	int i;

	flags &= ~(TASKQ_DYNAMIC | TASKQ_THREADS_CPU_PCT | TASKQ_DC_BATCH);

	tqs = kmem_zalloc(sizeof (*tqs) * nthreads, KM_SLEEP);
	kthreads = kmem_zalloc(sizeof (*kthreads) * nthreads, KM_SLEEP);

	tq = taskq_create(name, nthreads, minclsyspri, nthreads, INT_MAX,
	    flags | TASKQ_PREPOPULATE);
	VERIFY(tq != NULL);

	/* Spawn all sync threads */
	for (i = 0; i < nthreads; i++) {
		cv_init(&tqs[i].tqa_cv, NULL, CV_DEFAULT, NULL);
		mutex_init(&tqs[i].tqa_lock, NULL, MUTEX_DEFAULT, NULL);
		(void) taskq_dispatch(tq, taskq_sync_assign,
		    &tqs[i], TQ_FRONT);
	}

	/* Wait for all sync threads to start */
	for (i = 0; i < nthreads; i++) {
		mutex_enter(&tqs[i].tqa_lock);
		while (tqs[i].tqa_ready == 0)
			cv_wait(&tqs[i].tqa_cv, &tqs[i].tqa_lock);
		mutex_exit(&tqs[i].tqa_lock);
	}

	/* Let all sync threads resume and finish */
	for (i = 0; i < nthreads; i++) {
		mutex_enter(&tqs[i].tqa_lock);
		tqs[i].tqa_ready = 2;
		cv_broadcast(&tqs[i].tqa_cv);
		mutex_exit(&tqs[i].tqa_lock);
	}
	taskq_wait(tq);

	for (i = 0; i < nthreads; i++) {
		kthreads[i] = tqs[i].tqa_thread;
		mutex_destroy(&tqs[i].tqa_lock);
		cv_destroy(&tqs[i].tqa_cv);
	}
	kmem_free(tqs, sizeof (*tqs) * nthreads);

	*ktpp = kthreads;
	return (tq);
}

void
spl_taskq_init(void)
{
	system_delay_taskq = taskq_create("system_delay_taskq",
	    boot_ncpus, minclsyspri, 0, 0, 0);
}

void
spl_taskq_fini(void)
{
	if (system_delay_taskq != NULL) {
		taskq_destroy(system_delay_taskq);
		system_delay_taskq = NULL;
	}
}

/*
 * OpenZFS taskq extensions not present in native illumos taskq.
 */
int
taskq_cancel_id(taskq_t *tq, taskqid_t id)
{
	(void) tq;
	(void) id;
	return (ENOENT);
}

void
taskq_empty_ent(taskq_ent_t *t)
{
	(void) t;
}

boolean_t
taskq_of_curthread(taskq_t *tq)
{
	(void) tq;
	return (taskq_member(tq, curthread));
}

void
taskq_wait_outstanding(taskq_t *tq, taskqid_t id)
{
	(void) id;
	taskq_wait(tq);
}
