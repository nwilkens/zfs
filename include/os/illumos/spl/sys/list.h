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

#ifndef _SPL_SYS_LIST_H
#define	_SPL_SYS_LIST_H

/*
 * illumos provides list_t, list_node_t, list_create, list_destroy,
 * list_insert_after, list_insert_before, list_insert_head, list_insert_tail,
 * list_remove, list_remove_head, list_remove_tail, list_move_tail,
 * list_head, list_tail, list_next, list_prev, list_is_empty,
 * list_link_init, list_link_replace, list_link_active natively.
 *
 * Chain to the real header.
 */
#include_next <sys/list.h>

#endif	/* _SPL_SYS_LIST_H */
