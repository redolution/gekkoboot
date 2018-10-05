#include <_ansi.h>
#include <_syslist.h>
#ifndef REENTRANT_SYSCALLS_PROVIDED
#include <reent.h>
#endif
#include <errno.h>
#undef errno
extern int errno;

#include "asm.h"
#include "processor.h"

#define MEMLOCK_MUTEX_ID			0x00030040

static int initialized = 0;

void __memlock_init(void)
{
}

#ifndef REENTRANT_SYSCALLS_PROVIDED
void __libogc_malloc_lock(struct _reent *r)
{
}

void __libogc_malloc_unlock(struct _reent *r)
{
}

#else
void __libogc_malloc_lock(struct _reent *ptr)
{
}

void __libogc_malloc_unlock(struct _reent *ptr)
{
}

#endif
