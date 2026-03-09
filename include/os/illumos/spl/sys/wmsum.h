// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
 */

/*
 * wmsum counters are a reduced version of aggsum counters, optimized for
 * write-mostly scenarios.  They do not provide optimized read functions,
 * but instead allow much cheaper add function.  The primary usage is
 * infrequently read statistic counters, not requiring exact precision.
 *
 * On illumos, wmsum is implemented using a simple atomic uint64 with
 * memory barriers.  For higher-performance implementations, percpu
 * counters could be used, but the simple approach is correct and
 * sufficient for most statistical uses.
 */

#ifndef	_SYS_WMSUM_H
#define	_SYS_WMSUM_H

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef uint64_t wmsum_t;

static inline void
wmsum_init(wmsum_t *ws, uint64_t value)
{
	*ws = value;
}

static inline void
wmsum_fini(wmsum_t *ws)
{
	/* nothing to free for the simple implementation */
	(void) ws;
}

static inline uint64_t
wmsum_value(wmsum_t *ws)
{
	return (*ws);
}

static inline void
wmsum_add(wmsum_t *ws, int64_t delta)
{
	if (delta >= 0)
		atomic_add_64(ws, (uint64_t)delta);
	else
		atomic_add_64(ws, (uint64_t)(-(uint64_t)(-delta)));
}

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_WMSUM_H */
