/*
 * Institute for System Programming of the Russian Academy of Sciences
 * Copyright (C) 2016 ISPRAS
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, Version 3.
 *
 * This program is distributed in the hope # that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License version 3 for more details.
 */

#include <asp/arch.h>
#include <asp/time.h>
#include <asp/entries.h>
#include <assert.h>
#include <bsp/bsp.h>
#include "regs.h"
#include <arch/mmu.h>
#include "space.h"
#include <arch/memlayout.h>


extern char __vector_table_start[];
extern char __vector_table_end[];
extern char vector_table[];

void floating_point_enable(void);

//copy interrupt vector table to 0 virtual address
static void copy_vector_table(void)
{
    printf("copy vector table to %p, from %p, size (0x%x)\n",
            (void *)VECTOR_HIGH_VADDR, __vector_table_start, __vector_table_end - __vector_table_start);
    memcpy((void *)VECTOR_HIGH_VADDR, __vector_table_start, __vector_table_end - __vector_table_start);

    sctlr_set(sctlr_get()|SCTLR_V);

    // Ask linker to not throw away exceptions.o from libkernel.a
    int tmp = vector_table[0];
    (void) tmp;
}

void print_mainid()
{
    uint32_t midr, part;
    midr = midr_get();
    part = (midr>>4)&0xfff;
    /* part values:
        0xc08 is Cortex-A8
        0xc09 is Cortex-A9
        0xc05 is Cortex-A5
        0xc07 is Cortex-A7
        0xc0d is Cortex-A12
        0xc0e is Cortex-A17
        0xc0f is Cortex-A15
        0xd07 is Cortex-A57 - ARMv8
        0xd03 is Cortex-A53 - ARMv8
    */
    printf("Main ID Register (MIDR) = 0x%lx (part = 0x%lx)\n", midr, part);
}
void jet_arch_init(void)
{
    jet_console_init_all ();
    print_mainid();

    copy_vector_table();
    space_init(); //user space init
    ja_bsp_init();


    //enable access to coprocessors 10 and 11 for user and kernel
    //nsacr_set(NSACR_CP10|NSACR_CP11);
    cpacr_set(CPACR_CP10(CPACR_ACCESS_FULL) | CPACR_CP11(CPACR_ACCESS_FULL));
    floating_point_enable();

    //ja_preempt_enable();
    //int64_t time, time_old = 0;
    //while (1) {
    //    time = ja_system_time();
    //        //printf("%lld \n", time);
    //    if ((time - time_old) > 1000000000) {
    //        printf("%lld \n", time);
    //        time_old = time;
    //    }
    //}
}

void ja_preempt_disable (void)
{
    cpsr_set(cpsr_get() | (CPSR_IRQ));
}

void ja_preempt_enable (void)
{
    cpsr_set(cpsr_get() & ~(CPSR_IRQ));
}

pok_bool_t ja_preempt_enabled(void)
{
    return !(cpsr_get() & CPSR_IRQ);
}

void ja_inf_loop(void)
{
    while (1);
}

void ja_cpu_reset(void)
{
    assert(0);
}