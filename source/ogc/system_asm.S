#include <asm.h>

	.globl __realmode
__realmode:
	clrlwi	%r3,%r3,2
	mtsrr0	%r3
	mfmsr	%r3
	rlwinm	%r3,%r3,0,28,25
	mtsrr1	%r3
	rfi

	// complete init sequence taken from bootmii's ppc skeleton. thanks to segher
	// after a talk with dhewg we came to that point that it's good to wipe+setup BATS correctly
	.globl __configBATS
__configBATS:
	// HID0 = 00110c64:
	// bus checkstops off, sleep modes off,
	// caches off, caches invalidate,
	// store gathering off, enable data cache
	// flush assist, enable branch target cache,
	// enable branch history table
	lis		%r3,0x0011
	ori		%r3,%r3,0x0c64
	mtspr	HID0,%r3
	isync
	
#if defined(HW_RVL)
	lis		%r3,0x8200		//bits set: H4A(HID4 access), SBE(2nd BAT enabled)
	mtspr	HID4,%r3
	isync
#endif

	// clear all BATs
	li		%r0,0
	mtspr	IBAT0U,%r0; mtspr	IBAT1U,%r0; mtspr	IBAT2U,%r0; mtspr	IBAT3U,%r0		// IBAT0...3
	mtspr	DBAT0U,%r0; mtspr	DBAT1U,%r0; mtspr	DBAT2U,%r0; mtspr	DBAT3U,%r0		// DBAT0...3
#if defined(HW_RVL)
	mtspr	IBAT4U,%r0; mtspr	IBAT5U,%r0; mtspr	IBAT6U,%r0; mtspr	IBAT7U,%r0		// IBAT4...7
	mtspr	DBAT4U,%r0; mtspr	DBAT5U,%r0; mtspr	DBAT6U,%r0; mtspr	DBAT7U,%r0		// DBAT4...7
#endif
	isync

	// clear all SRs
	lis		%r0,0x8000
	mtsr	0,%r0; mtsr	1,%r0; mtsr	2,%r0; mtsr	3,%r0; mtsr	4,%r0; mtsr	5,%r0; mtsr	6,%r0
	mtsr	7,%r0; mtsr	8,%r0; mtsr	9,%r0; mtsr	10,%r0; mtsr	11,%r0; mtsr	12,%r0; mtsr	13,%r0
	mtsr	14,%r0; mtsr	15,%r0
	isync

	// set [DI]BAT0 for 256MB@80000000,
	// real 00000000, WIMG=0000, R/W
	li		%r3,2
	lis		%r4,0x8000
	ori		%r4,%r4,0x1fff
	mtspr	IBAT0L,%r3
	mtspr	IBAT0U,%r4
	mtspr	DBAT0L,%r3
	mtspr	DBAT0U,%r4
	isync
	
#if defined(HW_RVL)
	// set [DI]BAT4 for 256MB@90000000,
	// real 10000000, WIMG=0000, R/W
	addis	%r3,%r3,0x1000
	addis	%r4,%r4,0x1000
	mtspr	IBAT4L,%r3
	mtspr	IBAT4U,%r4
	mtspr	DBAT4L,%r3
	mtspr	DBAT4U,%r4
	isync
#endif
	
	// set DBAT1 for 256MB@c0000000,
	// real 00000000, WIMG=0101, R/W
	li		%r3,0x2a
	lis		%r4,0xc000
	ori		%r4,%r4,0x1fff
	mtspr	DBAT1L,%r3
	mtspr	DBAT1U,%r4
	isync
	
#if defined(HW_RVL)
	// set DBAT5 for 256MB@d0000000,
	// real 10000000, WIMG=0101, R/W
	addis	%r3,%r3,0x1000
	addis	%r4,%r4,0x1000
	mtspr	DBAT5L,%r3
	mtspr	DBAT5U,%r4
	isync
#endif

	mfmsr	%r3
	ori		%r3,%r3,MSR_DR|MSR_IR
	mtsrr1	%r3
	mflr	%r3
	oris    %r3,%r3,0x8000
	mtsrr0	%r3
	rfi
	
	.extern ICFlashInvalidate
	.globl __InitPS
__InitPS:
	mflr	%r0
	stw		%r0,4(%r1)
	stwu	%r1,-8(%r1)
	mfspr	%r3,HID2
	oris	%r3,%r3,0xA000
	mtspr	HID2,%r3
	isync
	bl		ICFlashInvalidate
	sync
	li		%r3,0
	mtspr	GQR0,%r3
	mtspr	GQR1,%r3
	mtspr	GQR2,%r3
	mtspr	GQR3,%r3
	mtspr	GQR4,%r3
	mtspr	GQR5,%r3
	mtspr	GQR6,%r3
	mtspr	GQR7,%r3
	isync
	lwz		%r0,12(%r1)
	addi	%r1,%r1,8
	mtlr	%r0
	blr

	.extern ICEnable
	.extern DCEnable
	.extern L2Init
	.extern L2Enable
	.globl __InitCache
__InitCache:
	mflr    %r0
	stw     %r0, 4(%r1)
	stwu    %r1, -16(%r1)
	stw     %r31, 12(%r1)

	mfspr   %r3,HID0
	rlwinm. %r0,%r3, 0, 16, 16	// Check if the Instruction Cache has been enabled or not.
	bne     ICEnabled

	bl		ICEnable
ICEnabled:
	mfspr   %r3, HID0
	rlwinm. %r0, %r3, 0, 17, 17	// Check if the Data Cache has been enabled or not.
	bne     DCEnabled
	
	bl		DCEnable	
DCEnabled:
	
	mfspr   %r3, L2CR
	clrrwi. %r0, %r3, 31			// Check if the Locked Cache has been enabled or not.
	bne     L2Enabled

	bl		L2Init
	bl		L2Enable

L2Enabled:
	# Restore the non-volatile registers to their previous values and return.
	lwz     %r0, 20(%r1)
	lwz     %r31, 12(%r1)
	addi    %r1, %r1, 16
	mtlr    %r0
	blr

	.globl __InitSystem
__InitSystem:
	mflr    %r0
	stw     %r0, 4(%r1)
	stwu    %r1, -24(%r1)
	stmw	%r29, 12(%r1)

	# Disable interrupts!
	mfmsr   %r3
	rlwinm  %r4,%r3,0,17,15
	rlwinm  %r4,%r4,0,26,24
	mtmsr   %r4

	# Clear various SPR's
	li      %r3,0
	mtspr   952, %r3
	mtspr   956, %r3
	mtspr   953, %r3
	mtspr   954, %r3
	mtspr   957, %r3
	mtspr   958, %r3
	isync
	
#if defined(HW_RVL)
	mfspr	%r3,HID4
	oris	%r3,%r3,0x0190		//set additional bits in HID4: SR0(store 0), LPE(PS LE exception), L2CFI(L2 castout prior to L2 inv. flash)
	mtspr	HID4,%r3
	isync
#endif

	# Disable Speculative Bus Accesses to non-guarded space from both caches.
	mfspr   %r3, HID0
	ori     %r3, %r3, 0x0200
	mtspr   HID0, %r3
	isync
	
	# Set the Non-IEEE mode in the FPSCR
	mtfsb1  29
	
	# Disable Write Gather Pipe
	mfspr   %r3,HID2 # (HID2)
	rlwinm  %r3, %r3, 0, 2, 0	
	mtspr   HID2,%r3 # (HID2)
	isync
	
	# Restore the non-volatile registers to their previous values and return.
	lwz     %r0, 28(%r1)
	lmw	%r29,12(%r1)
	addi    %r1, %r1, 24
	mtlr    %r0
	blr

	.globl __flush_cache
__flush_cache:
	lis		%r5,0xffff
	ori		%r5,%r5,0xfff1
	and		%r5,%r5,%r3
	subf	%r3,%r5,%r3
	add		%r4,%r4,%r3
1:	dcbst	%r0,%r5
	sync
	icbi	%r0,%r5
	addic	%r5,%r5,8
	subic.	%r4,%r4,8
	bge		1b
	sync
	blr

	.globl __reset
__reset:
	b		1f
9:	mfspr	%r8,HID0
	ori		%r8,%r8,0x0008
	mtspr	HID0,%r8
	isync
	sync
	nop
	b		2f
1:	b		3f
2:	mftb	%r5
4:	mftb	%r6
	subf	%r7,%r5,%r6
	cmplwi	%r7,0x1124
	blt		4b
	nop
	b		5f
3:	b		6f
5:	lis		%r8,0xCC00
	ori		%r8,%r8,0x3000
	li		%r4,3
	stw		%r4,36(%r8)
	stw		%r3,36(%r8)
	nop
	b		7f
6:	b		8f
7:	nop
	b		7b
8:	b		9b	

	.globl __InitBATS
__InitBATS:
	mflr	%r31
	oris	%r31,%r31,0x8000
	lis		%r3,__configBATS@h
	ori		%r3,%r3,__configBATS@l
	bl		__realmode
	mtlr	%r31
	blr
	
	.globl SYS_SwitchFiber
SYS_SwitchFiber:
	mflr	%r0
	mr		%r9,%r1
	stwu	%r9,-8(%r8)
	mr		%r1,%r8
	stw		%r0,4(%r9)
	mtlr	%r7
	blrl
	lwz		%r5,0(%r1)
	lwz		%r0,4(%r5)
	mtlr	%r0
	mr		%r1,%r5
	blr

	.section .data
	.balign	4
zeroF:
	.double 0.0
zeroPS:
	.float 0.0,0.0
