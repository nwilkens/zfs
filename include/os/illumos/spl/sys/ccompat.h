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

#ifndef _SPL_SYS_CCOMPAT_H
#define	_SPL_SYS_CCOMPAT_H

#include <sys/types.h>

/*
 * Compatibility definitions for OpenZFS common code.
 *
 * illumos provides most of these natively; we add only what is missing.
 */

/*
 * container_of - get pointer to enclosing structure.
 * illumos does not provide this; define it here.
 */
#ifndef container_of
/* BEGIN CSTYLED */
#define	container_of(ptr, type, member)				\
({								\
	const __typeof(((type *)0)->member) *__p = (ptr);	\
	(type *)((uintptr_t)__p - offsetof(type, member));	\
})
/* END CSTYLED */
#endif

/*
 * hlist - Linux-style hash list.  Not natively available on illumos.
 */
struct hlist_node {
	struct hlist_node *next, **pprev;
};

struct hlist_head {
	struct hlist_node *first;
};

/* BEGIN CSTYLED */
#define	HLIST_HEAD_INIT		{ }
#define	HLIST_HEAD(name)	struct hlist_head name = HLIST_HEAD_INIT
#define	INIT_HLIST_HEAD(head)	(head)->first = NULL

#define	INIT_HLIST_NODE(node)					\
	do {							\
		(node)->next = NULL;				\
		(node)->pprev = NULL;				\
	} while (0)
/* END CSTYLED */

#define	hlist_for_each(p, head) \
	for (p = (head)->first; p; p = (p)->next)

#define	hlist_entry(ptr, type, field)	container_of(ptr, type, field)

static inline void
hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	n->next = h->first;
	if (h->first != NULL)
		h->first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

static inline void
hlist_del(struct hlist_node *n)
{
	*(n->pprev) = n->next;
	if (n->next != NULL)
		n->next->pprev = n->pprev;
}

/*
 * atomic_t - simple atomic integer type.
 * Used in some OpenZFS common code paths.
 */
typedef struct {
	volatile int counter;
} atomic_t;

static inline int
atomic_read(const atomic_t *v)
{
	return (v->counter);
}

static inline int
atomic_inc(atomic_t *v)
{
	return (__sync_add_and_fetch(&v->counter, 1));
}

static inline int
atomic_dec(atomic_t *v)
{
	return (__sync_sub_and_fetch(&v->counter, 1));
}

#endif	/* _SPL_SYS_CCOMPAT_H */
