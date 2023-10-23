#include <gctypes.h>

vu32 _thread_dispatch_disable_level;
vu32 _context_switch_want;
void **__lwp_thr_libc_reent = NULL;
void *_thr_executing = NULL;

typedef struct _lwpnode {
	struct _lwpnode *next;
	struct _lwpnode *prev;
} lwp_node;

typedef struct _lwpqueue {
	lwp_node *first;
	lwp_node *perm_null;
	lwp_node *last;
} lwp_queue;
lwp_queue _wd_ticks_queue;

void __lwp_threadqueue_enqueuepriority() {}
void __lwp_threadqueue_dequeuepriority() {}
void __lwp_heap_free() {}
void __lwp_objmgr_initinfo() {}
void __lwp_wd_insert() {}
void __lwp_threadqueue_extractpriority() {}
void __lwp_thread_close() {}
void __lwp_mutex_surrender() {}
void __lwp_threadqueue_enqueue() {}
void __lwp_heap_allocate() {}
void __lwp_thread_changepriority() {}
void __lwp_threadqueue_dequeuefifo() {}
void __lwp_thread_init() {}
void __lwp_threadqueue_enqueuefifo() {}
void __lwp_wd_tickle() {}
void __lwp_thread_setstate() {}
void __lwp_thread_clearstate() {}
void __lwp_mutex_seize_irq_blocking() {}
void __lwp_threadqueue_extractfifo() {}
void __lwp_wd_remove() {}
void __lwp_thread_ready() {}
void __lwp_heap_init() {}
void __lwp_wkspace_init() {}
void __lwp_thread_settransient() {}
void __lwp_mutex_initialize() {}
void __lwp_thread_loadenv() {}
void __lwp_objmgr_get() {}
void __lwp_threadqueue_init() {}
void __lwp_objmgr_allocate() {}
void __lwp_thread_setpriority() {}
void __lwp_thread_stopmultitasking() {}
void __lwp_objmgr_getisrdisable() {}
void __lwp_objmgr_free() {}
void __lwp_stack_allocate() {}
void __lwp_queue_get() {}
void __lwp_thread_start() {}
void __lwp_queue_append() {}
void __lwp_thread_delayended() {}
void __lwp_queue_initialize() {}
void __lwp_threadqueue_flush() {}
void __lwp_threadqueue_extractproxy() {}
void __lwp_thread_exit() {}
void __lwp_watchdog_init() {}
void __lwp_threadqueue_dequeue() {}
void __lwp_objmgr_getnoprotection() {}
void __lwp_threadqueue_extract() {}
void __lwp_stack_free() {}
void __lwp_priority_init() {}
void __lwp_isr_in_progress() {}
void __lwp_mutex_flush() {}

void __thread_dispatch() {}
void __thread_dispatch_fp() {}
void __lwp_sysinit() {}
void __lwp_thread_setlibcreent() {}
void __lwp_thread_closeall() {}
void __lwp_thread_coreinit() {}
void __lwp_thread_startmultitasking() {}

void LWP_ThreadBroadcast() {}
void LWP_InitQueue() {}
void LWP_ThreadSleep() {}
