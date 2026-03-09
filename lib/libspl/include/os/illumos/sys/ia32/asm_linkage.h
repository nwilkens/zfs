// SPDX-License-Identifier: CDDL-1.0
/*
 * illumos asm_linkage.h for OpenZFS userland build.
 *
 * The system <ia32/sys/asm_linkage.h> guards macros with #ifdef _ASM,
 * but the OpenZFS userland build does not define _ASM when compiling .S
 * files.  Provide the macros unconditionally (matching FreeBSD pattern).
 */

#ifndef _LIBSPL_ILLUMOS_IA32_SYS_ASM_LINKAGE_H
#define	_LIBSPL_ILLUMOS_IA32_SYS_ASM_LINKAGE_H

#define	RET	ret

#undef ASMABI
#define	ASMABI	__attribute__((sysv_abi))

#define	ENDBR

#define	SECTION_TEXT .text
#define	SECTION_STATIC .section .rodata

#if defined(__amd64)
#define	CLONGSHIFT	3
#define	CLONGSIZE	8
#define	CLONGMASK	7
#elif defined(__i386)
#define	CLONGSHIFT	2
#define	CLONGSIZE	4
#define	CLONGMASK	3
#endif

#define	CPTRSHIFT	CLONGSHIFT
#define	CPTRSIZE	CLONGSIZE
#define	CPTRMASK	CLONGMASK

#define	ASM_ENTRY_ALIGN	16

#define	XMM_SIZE	16
#define	XMM_ALIGN	16

#define	ENTRY(x) \
	.text; \
	.balign	ASM_ENTRY_ALIGN; \
	.globl	x; \
	.type	x, @function; \
x:

#define	ENTRY_NP(x) \
	.text; \
	.balign	ASM_ENTRY_ALIGN; \
	.globl	x; \
	.type	x, @function; \
x:

#define	ENTRY_ALIGN(x, a) \
	.text; \
	.balign	a; \
	.globl	x; \
	.type	x, @function; \
x:

#define	FUNCTION(x) \
	.type	x, @function; \
x:

#define	ENTRY2(x, y) \
	.text; \
	.balign	ASM_ENTRY_ALIGN; \
	.globl	x, y; \
	.type	x, @function; \
	.type	y, @function; \
x:; \
y:

#define	ENTRY_NP2(x, y) \
	.text; \
	.balign	ASM_ENTRY_ALIGN; \
	.globl	x, y; \
	.type	x, @function; \
	.type	y, @function; \
x:; \
y:

#define	SET_SIZE(x) \
	.size	x, [.-x]

#define	SET_OBJ(x)

/*
 * GAS syntax helpers
 */
#if defined(__GNUC_AS__) || defined(__GNUC__)
#define	D16	.byte	0x66;
#define	A16	.byte	0x67;
#define	_CONST(const)		(const)
#define	_BITNOT(const)		~_CONST(const)
#define	_MUL(a, b)		_CONST(a * b)
#else
#define	D16	data16;
#define	A16	addr16;
#define	_CONST(const)		[const]
#define	_BITNOT(const)		-1!_CONST(const)
#define	_MUL(a, b)		_CONST(a \* b)
#endif

#endif /* _LIBSPL_ILLUMOS_IA32_SYS_ASM_LINKAGE_H */
