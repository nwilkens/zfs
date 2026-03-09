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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

/*
 * SPL string functions for illumos kernel.
 *
 * kmem_strdup, kmem_strfree, kmem_asprintf, kmem_vasprintf,
 * kmem_scnprintf, strident_canon — needed by OpenZFS but not
 * provided by the base illumos kernel.
 */

#include <sys/zfs_context.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/varargs.h>

char *
kmem_strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	char *buf;

	buf = kmem_alloc(len, KM_SLEEP);
	bcopy(s, buf, len);
	return (buf);
}

void
kmem_strfree(char *str)
{
	ASSERT3P(str, !=, NULL);
	kmem_free(str, strlen(str) + 1);
}

/*
 * Do not change the length of the returned string; it must be freed
 * with kmem_strfree().
 */
char *
kmem_asprintf(const char *fmt, ...)
{
	int size;
	va_list adx;
	char *buf;

	va_start(adx, fmt);
	size = vsnprintf(NULL, 0, fmt, adx) + 1;
	va_end(adx);

	buf = kmem_alloc(size, KM_SLEEP);

	va_start(adx, fmt);
	(void) vsnprintf(buf, size, fmt, adx);
	va_end(adx);

	return (buf);
}

char *
kmem_vasprintf(const char *fmt, va_list adx)
{
	char *msg;
	va_list adx2;

	va_copy(adx2, adx);
	msg = kmem_alloc(vsnprintf(NULL, 0, fmt, adx) + 1, KM_SLEEP);
	(void) vsprintf(msg, fmt, adx2);
	va_end(adx2);

	return (msg);
}

/*
 * kmem_scnprintf() returns the number of characters actually printed
 * (excluding NUL), capped at size-1.  This is the "safe" version that
 * avoids the snprintf() quirk of returning the would-be length.
 */
int
kmem_scnprintf(char *restrict buf, size_t size,
    const char *restrict fmt, ...)
{
	int n;
	va_list ap;

	if (size == 0)
		return (0);

	va_start(ap, fmt);
	n = vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	if (n >= size)
		n = size - 1;

	return (n);
}

#define	IS_DIGIT(c)	((c) >= '0' && (c) <= '9')
#define	IS_ALPHA(c)	\
	(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

/*
 * Convert a string into a valid C identifier by replacing invalid
 * characters with '_'.  Also makes sure the string is NUL-terminated
 * and takes up at most n bytes.
 */
void
strident_canon(char *s, size_t n)
{
	char c;
	char *end = s + n - 1;

	if ((c = *s) == 0)
		return;

	if (!IS_ALPHA(c) && c != '_')
		*s = '_';

	while (s < end && ((c = *(++s)) != 0)) {
		if (!IS_ALPHA(c) && !IS_DIGIT(c) && c != '_')
			*s = '_';
	}
	*s = 0;
}
