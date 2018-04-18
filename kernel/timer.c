/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#include <stddef.h>
#include <time.h>
#include "kernel.h"
#include "fixedptc.h"

/*记录系统启动以来，定时器中断的次数*/
unsigned volatile g_timer_ticks = 0;

// 系统的平均负载
fixedpt g_load_avg = 0;

/**
 * 定时器的中断处理程序
 */
void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    // sys_putchar('.');
    
    // 每秒更新所有的线程状态
    if(g_timer_ticks % HZ == 0)
    {
        // printk("timer ticks: %d\n", g_timer_ticks);

        fixedpt ratio;
        unsigned readyCnt = 0;

        // 统计ready状态的线程数量
        for (struct tcb *tsk = g_task_head; tsk != NULL; tsk =tsk->next)
            if(tsk->state == TASK_STATE_READY && tsk->tid != 0)
                readyCnt++;

        // 计算平均负载
        fixedpt r59_60 = fixedpt_div(fixedpt_fromint(59), fixedpt_fromint(60));
        fixedpt r01_60 = fixedpt_div(FIXEDPT_ONE,         fixedpt_fromint(60));
        g_load_avg = fixedpt_add(fixedpt_mul(r59_60, g_load_avg),
                                 fixedpt_mul(r01_60, fixedpt_fromint(readyCnt)));

        // 更新所有线程的CPU时间
        for (struct tcb *tsk = g_task_head; tsk != NULL; tsk = tsk->next)
        {
            ratio = fixedpt_mul(FIXEDPT_TWO, g_load_avg);
            ratio = fixedpt_div(ratio, fixedpt_add(ratio, FIXEDPT_ONE));
            tsk->estcpu = fixedpt_add(fixedpt_mul(ratio, tsk->estcpu), fixedpt_fromint(tsk->nice));
            // printk("task #%d : %d\n", tsk->tid, tsk->state);
        }

        // printk("read count : %d\n\n", readyCnt);
    }

    if(g_task_running != NULL) {
        //如果是task0在运行，则强制调度
        if(g_task_running->tid == 0) {
            g_resched = 1;
        } else {
            //否则，把当前线程的时间片减一
            --g_task_running->timeslice;
            
            // 当前线程的CPU时间加一
            if(g_task_running->tid != 0)
                g_task_running->estcpu = fixedpt_add(g_task_running->estcpu, FIXEDPT_ONE);
           
            // sys_putchar(g_task_running->estcpu);

            //如果当前线程用完了时间片，也要强制调度1
            if(g_task_running->timeslice <= 0) {
                g_resched = 1;
                g_task_running->timeslice = TASK_TIMESLICE_DEFAULT;
            }
        }
    }
}

#define barrier() __asm__ __volatile__ ("" : : : "memory")
static void busy_wait(unsigned loops)
{
  do {
    barrier ();
  } while (loops-- > 0);
}

/* Number of loops per timer tick.
   Initialized by calibrate_delay(). */
static unsigned loops_per_tick;

/* This is the number of bits of precision for the loops_per_tick.  Each
 * bit takes on average 1.5/HZ seconds.  This (like the original) is a little
 * better than 1% */
#define LPS_PREC 8

void calibrate_delay(void)
{
    unsigned long tmp, loopbit;
    int lps_precision = LPS_PREC;

    loops_per_tick = (1<<12);

    printk("Calibrating delay... ");

    while (loops_per_tick <<= 1) {
        /* wait for "start of" clock tick */
        tmp = g_timer_ticks;
        while (tmp == g_timer_ticks)
            /* nothing */;
        /* Go .. */
        tmp = g_timer_ticks;
        busy_wait(loops_per_tick);

        tmp = g_timer_ticks - tmp;
        if (tmp)
            break;
    }

    /* Do a binary approximation to get loops_per_tick set to equal one clock
     * (up to lps_precision bits)
     */
    loops_per_tick >>= 1;
    loopbit = loops_per_tick;
    while ( lps_precision-- && (loopbit >>= 1) ) {
        loops_per_tick |= loopbit;

        /* wait for "start of" clock tick */
        tmp = g_timer_ticks;
        while (tmp == g_timer_ticks)
            /* nothing */;
        /* Go .. */
        tmp = g_timer_ticks;
        busy_wait(loops_per_tick);

        if (g_timer_ticks != tmp)   /* longer than 1 tick */
            loops_per_tick &= ~loopbit;
    }

    printk ("%u loops per second (%lu.%02lu BogoMIPS)\r\n",
            loops_per_tick * HZ,
            loops_per_tick/(500000/HZ),
            loops_per_tick/(5000/HZ) % 100);
}

static unsigned _sleep(unsigned ticks)
{
  unsigned start = g_timer_ticks;

  while (g_timer_ticks - start < ticks)
    sys_task_yield ();

  return 0;
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void _delay (unsigned num, unsigned denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  busy_wait (loops_per_tick * num / 1000 * HZ / (denom / 1000));
}

static void do_sleep (unsigned num, unsigned denom)
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.

        (NUM / DENOM) s
     ---------------------- = NUM * HZ / DENOM ticks.
        1 s / HZ ticks
  */
  unsigned ticks = num * HZ / denom;

  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         sys_sleep() because it will yield the CPU to other
         processes. */
      _sleep (ticks);
    }
  else
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      _delay (num, denom);
    }
}

unsigned sys_sleep(unsigned seconds)
{
    do_sleep(seconds, 1);
    return 0;
}

int sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    if(rqtp->tv_sec > 0)
        do_sleep(rqtp->tv_sec, 1);
    if(rqtp->tv_nsec > 0)
        do_sleep(rqtp->tv_nsec, 1000 * 1000 * 1000);
    return 0;
}
