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
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * OS-specific event support for illumos.
 * illumos uses native sysevents via ddi_sysevent; no OS-specific
 * event plumbing is needed here.
 */

/*
 * ZSTD trace hooks.  These are declared with __weak__ in the ZSTD library
 * but the weak attribute doesn't work reliably in kernel modules.
 * Provide no-op implementations.
 */
typedef unsigned long long ZSTD_TraceCtx;
struct ZSTD_CCtx_s;
struct ZSTD_DCtx_s;

ZSTD_TraceCtx
ZSTD_trace_compress_begin(struct ZSTD_CCtx_s const *cctx)
{
	(void) cctx;
	return (0);
}

void
ZSTD_trace_compress_end(struct ZSTD_CCtx_s const *cctx, ZSTD_TraceCtx ctx,
    unsigned long long size)
{
	(void) cctx;
	(void) ctx;
	(void) size;
}

ZSTD_TraceCtx
ZSTD_trace_decompress_begin(struct ZSTD_DCtx_s const *dctx)
{
	(void) dctx;
	return (0);
}

void
ZSTD_trace_decompress_end(struct ZSTD_DCtx_s const *dctx, ZSTD_TraceCtx ctx,
    unsigned long long size, int errorCode)
{
	(void) dctx;
	(void) ctx;
	(void) size;
	(void) errorCode;
}
