#ifndef __TIME_H__
#define __TIME_H__

#define TB_CORE_CLOCK				486000000u
#define TB_BUS_CLOCK				162000000u
#define TB_TIMER_CLOCK				(TB_BUS_CLOCK/4000)			//4th of the bus frequency

#define ticks_to_secs(ticks)		(((u64)(ticks)/(u64)(TB_TIMER_CLOCK*1000)))
#define secs_to_ticks(sec)			((u64)(sec)*(TB_TIMER_CLOCK*1000))

extern void settime(u64);

#endif
