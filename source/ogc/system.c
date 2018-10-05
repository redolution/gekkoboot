/*-------------------------------------------------------------

system.c -- OS functions and initialization

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

//#define DEBUG_SYSTEM

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/iosupport.h>

#include "asm.h"
#include "irq.h"
#include "processor.h"
#include "ogc/exi.h"
#include "ogc/cache.h"
#include "ogc/video.h"
#include "ogc/time.h"
#include "ogc/system.h"

#define SYSMEM1_SIZE				0x01800000
#define KERNEL_HEAP					(1*1024*1024)

#define _SHIFTL(v, s, w)	\
    ((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

struct _sramcntrl {
	u8 srambuf[64];
	u32 offset;
	s32 enabled;
	s32 locked;
	s32 sync;
} sramcntrl ATTRIBUTE_ALIGN(32);

static u32 system_initialized = 0;

static void *__sysarena1lo = NULL;
static void *__sysarena1hi = NULL;

static vu16* const _viReg = (u16*)0xCC002000;
static vu32* const _piReg = (u32*)0xCC003000;
static vu16* const _memReg = (u16*)0xCC004000;

void* SYS_AllocArena1MemLo(u32 size,u32 align);

static s32 __sram_sync(void);
static s32 __sram_writecallback(s32 chn,s32 dev);

extern void __heap_init(void);
extern void __exception_init(void);
extern void __exception_closeall(void);
extern void __systemcall_init(void);
extern void __decrementer_init(void);
extern void __exi_init(void);
extern void __si_init(void);
extern void __irq_init(void);
extern void __timesystem_init(void);
extern void __memlock_init(void);
extern void __libc_init(int);

extern void __libogc_malloc_lock( struct _reent *ptr );
extern void __libogc_malloc_unlock( struct _reent *ptr );

extern void __exception_console(void);
extern void __exception_printf(const char *str, ...);

extern void __realmode(void*);
extern void __configMEM1_24Mb(void);
extern void __configMEM1_48Mb(void);
extern void __configMEM2_64Mb(void);
extern void __configMEM2_128Mb(void);
extern void __reset(u32 reset_code);

extern u32 __IPC_ClntInit(void);

extern void __console_init_ex(void *conbuffer,int tgt_xstart,int tgt_ystart,int tgt_stride,int con_xres,int con_yres,int con_stride);

extern void timespec_subtract(const struct timespec *tp_start,const struct timespec *tp_end,struct timespec *result);


extern int __libogc_lock_init(int *lock,int recursive);
extern int __libogc_lock_close(int *lock);
extern int __libogc_lock_release(int *lock);
extern int __libogc_lock_acquire(int *lock);
extern void __libogc_exit(int status);
extern void * __libogc_sbrk_r(struct _reent *ptr, ptrdiff_t incr);
extern int __libogc_gettod_r(struct _reent *ptr, struct timeval *tp, struct timezone *tz);
extern int __libogc_nanosleep(const struct timespec *tb, struct timespec *rem);

extern u8 __gxregs[];
extern u8 __text_start[];
extern u8 __Arena1Lo[], __Arena1Hi[];

u8 *__argvArena1Lo = (u8*)0xdeadbeef;

#define SOFTRESET_ADR *((vu32*)0xCC003024)
void __reload(void) { SOFTRESET_ADR=0; }

void __libogc_exit(int status)
{
	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
}

static void __init_syscall_array(void) {
	__syscalls.sbrk_r = __libogc_sbrk_r;
	__syscalls.lock_init = __libogc_lock_init;
	__syscalls.lock_close = __libogc_lock_close;
	__syscalls.lock_release = __libogc_lock_release;
	__syscalls.lock_acquire = __libogc_lock_acquire;
	__syscalls.malloc_lock = __libogc_malloc_lock;
	__syscalls.malloc_unlock = __libogc_malloc_unlock;
	__syscalls.exit = __libogc_exit;
	__syscalls.gettod_r = __libogc_gettod_r;
	__syscalls.nanosleep = __libogc_nanosleep;

}

static void __dohotreset(u32 resetcode)
{
	u32 level;

	_CPU_ISR_Disable(level);
	_viReg[1] = 0;
	ICFlashInvalidate();
	__reset(resetcode<<3);
}

static s32 __call_resetfuncs(s32 final)
{
	s32 ret;

	ret = 1;
	if(__sram_sync()==0) ret |= (ret<<1);

	if(ret&~0x01) return 0;
	return 1;
}

static void __doreboot(u32 resetcode,s32 force_menu)
{
	u32 level;

	_CPU_ISR_Disable(level);

	*((u32*)0x817ffffc) = 0;
	*((u32*)0x817ffff8) = 0;
	*((u32*)0x800030e2) = 1;
}

static void __MEMInterruptHandler(u32 irq, void* ctx)
{
	_memReg[16] = 0;
}

void * __attribute__ ((weak)) __myArena1Lo = 0;
void * __attribute__ ((weak)) __myArena1Hi = 0;

static void __lowmem_init(void)
{
	u32 *_gx = (u32*)__gxregs;

	void *ram_start = (void*)0x80000000;
	void *ram_end = (void*)(0x80000000|SYSMEM1_SIZE);
	void *arena_start = (void*)0x80003000;

	memset(_gx,0,2048);
	memset(arena_start,0,0x100);
	if ( __argvArena1Lo == (u8*)0xdeadbeef ) __argvArena1Lo = __Arena1Lo;
	if (__myArena1Lo == 0) __myArena1Lo = __argvArena1Lo;
	if (__myArena1Hi == 0) __myArena1Hi = __Arena1Hi;

	memset(ram_start,0,0x100);
	*((u32*)(ram_start+0x20))	= 0x0d15ea5e;   // magic word "disease"
	*((u32*)(ram_start+0x24))	= 1;            // version
	*((u32*)(ram_start+0x28))	= SYSMEM1_SIZE;	// physical memory size
	*((u32*)(ram_start+0x2C))	= 1 + ((*(u32*)0xCC00302c)>>28);

	*((u32*)(ram_start+0x30))	= (u32)__myArena1Lo;
	*((u32*)(ram_start+0x34))	= (u32)__myArena1Hi;

	*((u32*)(ram_start+0xEC))	= (u32)ram_end;	// ram_end (??)
	*((u32*)(ram_start+0xF0))	= SYSMEM1_SIZE;	// simulated memory size
	*((u32*)(ram_start+0xF8))	= TB_BUS_CLOCK;	// bus speed: 162 MHz
	*((u32*)(ram_start+0xFC))	= TB_CORE_CLOCK;	// cpu speed: 486 Mhz

	*((u16*)(arena_start+0xE0))	= 6; // production pads
	*((u32*)(arena_start+0xE4))	= 0xC0008000;

	DCFlushRangeNoSync(ram_start, 0x100);

	DCFlushRangeNoSync(arena_start, 0x100);
	DCFlushRangeNoSync(_gx, 2048);
	_sync();

	SYS_SetArenaLo((void*)__myArena1Lo);
	SYS_SetArenaHi((void*)__myArena1Hi);
}

static void __memprotect_init(void)
{
	u32 level;

	_CPU_ISR_Disable(level);

	__MaskIrq((IM_MEM0|IM_MEM1|IM_MEM2|IM_MEM3));

	_memReg[16] = 0;
	_memReg[8] = 255;

	IRQ_Request(IRQ_MEM0,__MEMInterruptHandler,NULL);
	IRQ_Request(IRQ_MEM1,__MEMInterruptHandler,NULL);
	IRQ_Request(IRQ_MEM2,__MEMInterruptHandler,NULL);
	IRQ_Request(IRQ_MEM3,__MEMInterruptHandler,NULL);
	IRQ_Request(IRQ_MEMADDRESS,__MEMInterruptHandler,NULL);

	__UnmaskIrq(IM_MEMADDRESS);		//only enable memaddress irq atm

	_CPU_ISR_Restore(level);
}

static u32 __getrtc(u32 *gctime)
{
	u32 ret;
	u32 cmd;
	u32 time;

	if(EXI_Lock(EXI_CHANNEL_0,EXI_DEVICE_1,NULL)==0) return 0;
	if(EXI_Select(EXI_CHANNEL_0,EXI_DEVICE_1,EXI_SPEED8MHZ)==0) {
		EXI_Unlock(EXI_CHANNEL_0);
		return 0;
	}

	ret = 0;
	time = 0;
	cmd = 0x20000000;
	if(EXI_Imm(EXI_CHANNEL_0,&cmd,4,EXI_WRITE,NULL)==0) ret |= 0x01;
	if(EXI_Sync(EXI_CHANNEL_0)==0) ret |= 0x02;
	if(EXI_Imm(EXI_CHANNEL_0,&time,4,EXI_READ,NULL)==0) ret |= 0x04;
	if(EXI_Sync(EXI_CHANNEL_0)==0) ret |= 0x08;
	if(EXI_Deselect(EXI_CHANNEL_0)==0) ret |= 0x10;

	EXI_Unlock(EXI_CHANNEL_0);
	*gctime = time;
	if(ret) return 0;

	return 1;
}

static u32 __sram_read(void *buffer)
{
	u32 command,ret;

	DCInvalidateRange(buffer,64);

	if(EXI_Lock(EXI_CHANNEL_0,EXI_DEVICE_1,NULL)==0) return 0;
	if(EXI_Select(EXI_CHANNEL_0,EXI_DEVICE_1,EXI_SPEED8MHZ)==0) {
		EXI_Unlock(EXI_CHANNEL_0);
		return 0;
	}

	ret = 0;
	command = 0x20000100;
	if(EXI_Imm(EXI_CHANNEL_0,&command,4,EXI_WRITE,NULL)==0) ret |= 0x01;
	if(EXI_Sync(EXI_CHANNEL_0)==0) ret |= 0x02;
	if(EXI_Dma(EXI_CHANNEL_0,buffer,64,EXI_READ,NULL)==0) ret |= 0x04;
	if(EXI_Sync(EXI_CHANNEL_0)==0) ret |= 0x08;
	if(EXI_Deselect(EXI_CHANNEL_0)==0) ret |= 0x10;
	if(EXI_Unlock(EXI_CHANNEL_0)==0) ret |= 0x20;

	if(ret) return 0;
	return 1;
}

static u32 __sram_write(void *buffer,u32 loc,u32 len)
{
	u32 cmd,ret;

	if(EXI_Lock(EXI_CHANNEL_0,EXI_DEVICE_1,__sram_writecallback)==0) return 0;
	if(EXI_Select(EXI_CHANNEL_0,EXI_DEVICE_1,EXI_SPEED8MHZ)==0) {
		EXI_Unlock(EXI_CHANNEL_0);
		return 0;
	}

	ret = 0;
	cmd = 0xa0000100+(loc<<6);
	if(EXI_Imm(EXI_CHANNEL_0,&cmd,4,EXI_WRITE,NULL)==0) ret |= 0x01;
	if(EXI_Sync(EXI_CHANNEL_0)==0) ret |= 0x02;
	if(EXI_ImmEx(EXI_CHANNEL_0,buffer,len,EXI_WRITE)==0) ret |= 0x04;
	if(EXI_Deselect(EXI_CHANNEL_0)==0) ret |= 0x08;
	if(EXI_Unlock(EXI_CHANNEL_0)==0) ret |= 0x10;

	if(ret) return 0;
	return 1;
}

static s32 __sram_writecallback(s32 chn,s32 dev)
{
	sramcntrl.sync = __sram_write(sramcntrl.srambuf+sramcntrl.offset,sramcntrl.offset,(64-sramcntrl.offset));
	if(sramcntrl.sync) sramcntrl.offset = 64;

	return 1;
}

static s32 __sram_sync(void)
{
	return sramcntrl.sync;
}

void __sram_init(void)
{
	sramcntrl.enabled = 0;
	sramcntrl.locked = 0;
	sramcntrl.sync = __sram_read(sramcntrl.srambuf);

	sramcntrl.offset = 64;
}

static void DisableWriteGatherPipe(void)
{
	mtspr(920,(mfspr(920)&~0x40000000));
}

static void* __locksram(u32 loc)
{
	u32 level;

	_CPU_ISR_Disable(level);
	if(!sramcntrl.locked) {
		sramcntrl.enabled = level;
		sramcntrl.locked = 1;
		return (void*)((u32)sramcntrl.srambuf+loc);
	}
	_CPU_ISR_Restore(level);
	return NULL;
}

static u32 __unlocksram(u32 write,u32 loc)
{
	sramcntrl.locked = 0;
	_CPU_ISR_Restore(sramcntrl.enabled);
	return sramcntrl.sync;
}

syssram* __SYS_LockSram(void)
{
	return (syssram*)__locksram(0);
}

syssramex* __SYS_LockSramEx(void)
{
	return (syssramex*)__locksram(sizeof(syssram));
}

u32 __SYS_UnlockSram(u32 write)
{
	return __unlocksram(write,0);
}

u32 __SYS_UnlockSramEx(u32 write)
{
	return __unlocksram(write,sizeof(syssram));
}

u32 __SYS_SyncSram(void)
{
	return __sram_sync();
}

u32 __SYS_GetRTC(u32 *gctime)
{
	u32 cnt,ret;
	u32 time1,time2;

	cnt = 0;
	ret = 0;
	while(cnt<16) {
		if(__getrtc(&time1)==0) ret |= 0x01;
		if(__getrtc(&time2)==0) ret |= 0x02;
		if(ret) return 0;
		if(time1==time2) {
			*gctime = time1;
			return 1;
		}
		cnt++;
	}
	return 0;
}

void SYS_Init(void)
{
	u32 level;

	_CPU_ISR_Disable(level);

	if(system_initialized) return;
	system_initialized = 1;

	__init_syscall_array();
	__lowmem_init();
	__exception_init();
	__systemcall_init();
	__decrementer_init();
	__irq_init();
	__exi_init();
	__sram_init();
	__si_init();
	__memlock_init();
	__timesystem_init();

	__memprotect_init();

	DisableWriteGatherPipe();
	__libc_init(1);
	_CPU_ISR_Restore(level);
}

u32 SYS_ResetButtonDown(void)
{
	return (!(_piReg[0]&0x00010000));
}

void SYS_ResetSystem(s32 reset,u32 reset_code,s32 force_menu)
{
	u32 ret = 0;
	syssram *sram;

	while(__call_resetfuncs(FALSE)==0);

	if(reset==SYS_HOTRESET && force_menu==TRUE) {
		sram = __SYS_LockSram();
		sram->flags |= 0x40;
		__SYS_UnlockSram(TRUE);
		while(!__SYS_SyncSram());
	}

	__exception_closeall();
	__call_resetfuncs(TRUE);

	LCDisable();

	if(reset==SYS_HOTRESET) {
		__dohotreset(reset_code);
	} else if(reset==SYS_RESTART) {
		__doreboot(reset_code,force_menu);
	}

	memset((void*)0x80000040,0,140);
	memset((void*)0x800000D4,0,20);
	memset((void*)0x800000F4,0,4);
	memset((void*)0x80003000,0,192);
	memset((void*)0x800030C8,0,12);
	memset((void*)0x800030E2,0,1);
}

void SYS_SetArena1Lo(void *newLo)
{
	u32 level;

	_CPU_ISR_Disable(level);
	__sysarena1lo = newLo;
	_CPU_ISR_Restore(level);
}

void* SYS_GetArena1Lo(void)
{
	u32 level;
	void *arenalo;

	_CPU_ISR_Disable(level);
	arenalo = __sysarena1lo;
	_CPU_ISR_Restore(level);

	return arenalo;
}

void SYS_SetArena1Hi(void *newHi)
{
	u32 level;

	_CPU_ISR_Disable(level);
	__sysarena1hi = newHi;
	_CPU_ISR_Restore(level);
}

void* SYS_GetArena1Hi(void)
{
	u32 level;
	void *arenahi;

	_CPU_ISR_Disable(level);
	arenahi = __sysarena1hi;
	_CPU_ISR_Restore(level);

	return arenahi;
}

u32 SYS_GetArena1Size(void)
{
	u32 level,size;

	_CPU_ISR_Disable(level);
	size = ((u32)__sysarena1hi - (u32)__sysarena1lo);
	_CPU_ISR_Restore(level);

	return size;
}

void* SYS_AllocArena1MemLo(u32 size,u32 align)
{
	u32 mem1lo;
	void *ptr = NULL;

	mem1lo = (u32)SYS_GetArena1Lo();
	ptr = (void*)((mem1lo+(align-1))&~(align-1));
	mem1lo = ((((u32)ptr+size+align)-1)&~(align-1));
	SYS_SetArena1Lo((void*)mem1lo);

	return ptr;
}

void SYS_ProtectRange(u32 chan,void *addr,u32 bytes,u32 cntrl)
{
	u16 rcntrl;
	u32 pstart,pend,level;

	if(chan<SYS_PROTECTCHANMAX) {
		pstart = ((u32)addr)&~0x3ff;
		pend = ((((u32)addr)+bytes)+1023)&~0x3ff;
		DCFlushRange((void*)pstart,(pend-pstart));

		_CPU_ISR_Disable(level);

		__UnmaskIrq(IRQMASK(chan));
		_memReg[chan<<2] = _SHIFTR(pstart,10,16);
		_memReg[(chan<<2)+1] = _SHIFTR(pend,10,16);

		rcntrl = _memReg[8];
		rcntrl = (rcntrl&~(_SHIFTL(3,(chan<<1),2)))|(_SHIFTL(cntrl,(chan<<1),2));
		_memReg[8] = rcntrl;

		if(cntrl==SYS_PROTECTRDWR)
			__MaskIrq(IRQMASK(chan));


		_CPU_ISR_Restore(level);
	}
}

void* SYS_AllocateFramebuffer(GXRModeObj *rmode)
{
	return memalign(32, VIDEO_GetFrameBufferSize(rmode));
}

u64 SYS_Time(void)
{
	u64 current_time = 0;
    u32 gmtime =0;
    __SYS_GetRTC(&gmtime);
    current_time = gmtime;
	syssram* sram = __SYS_LockSram();
	current_time += sram->counter_bias;
	__SYS_UnlockSram(0);
	return (TB_TIMER_CLOCK * 1000) * current_time;
}
