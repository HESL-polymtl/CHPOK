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
 *
 * This file also incorporates work covered by POK License.
 * Copyright (c) 2007-2009 POK team
 */

#include <types.h>
#include <errno.h>
#include <libc.h>
#include <assert.h>

#include "interrupt.h"
#include <asp/arch.h>
#include <arch/deployment.h>

#include "gdt.h"
#include "tss.h"

#include "space.h"
#include <core/sched.h>

#include <asp/alloc.h>

#define KERNEL_STACK_SIZE 8192

void ja_space_layout_get(jet_space_id space_id,
    struct jet_space_layout* space_layout)
{
    assert(space_id != 0 && space_id <= ja_spaces_n);

    space_layout->kernel_addr = (char*)ja_spaces[space_id - 1].phys_base;
    space_layout->user_addr = (char* __user)POK_PARTITION_MEMORY_BASE;
    space_layout->size = ja_spaces[space_id - 1].size_normal;
}

struct jet_kernel_shared_data* __kuser ja_space_shared_data(jet_space_id space_id)
{
    struct ja_x86_space* space = &ja_spaces[space_id - 1];
    return (struct jet_kernel_shared_data* __kuser)space->phys_base;
}

/* FIX: Alexy Torres, variable unused, fix: comment */
#if 0
static const size_t ja_user_space_maximum_alignment = 16;
#endif
void __user* ja_space_get_heap(jet_space_id space_id)
{
   struct ja_x86_space* space = &ja_spaces[space_id - 1];

   return (void __user*)(space->heap_end - space->size_heap);
}


jet_space_id current_space_id = 0;

void ja_user_space_jump(
    jet_stack_t stack_kernel,
    jet_space_id space_id,
    void (__user * entry_user)(void),
    jet_ustack_t stack_user)
{
    /*
     * Reuse layout of interrupt_frame structure, allocated on stack,
     * for own purposes.
     * 
     * Usage of this structure here is unrelated to interrupts
     * because it is allocated not at the *beginning* of the stack.
     */
   interrupt_frame   ctx;
   uint32_t          code_sel;
   uint32_t          data_sel;
   uint32_t          sp;

   assert(space_id <= ja_spaces_n); //TODO: fix comparision

   code_sel = GDT_BUILD_SELECTOR (GDT_PARTITION_CODE_SEGMENT (space_id), 0, 3);
   data_sel = GDT_BUILD_SELECTOR (GDT_PARTITION_DATA_SEGMENT (space_id), 0, 3);

   sp = (uint32_t) &ctx;

   memset (&ctx, 0, sizeof (interrupt_frame));

   ctx.es = ctx.ds = ctx.ss = data_sel;

   // ctx.__esp   = (uint32_t) (&ctx.error); /* for pusha */
   ctx.eip     = (uint32_t)entry_user;
   //ctx.eax     = arg1;
   //ctx.ebx     = arg2;
   ctx.cs      = code_sel;
   ctx.eflags  = 1 << 9 | 3<<12;
   ctx.esp     = stack_user;

   tss_set_esp0 (stack_kernel);

   asm ("mov %0, %%esp		\n"
        "pop %%es		\n"
        "pop %%ds		\n"
        "popa			\n"
        "addl $4, %%esp		\n"
        "iret			\n"
        :
        : "m" (sp)
       );
}

static void ja_space_create (jet_space_id space_id,
                            uintptr_t addr,
                            size_t size)
{
   gdt_set_segment (GDT_PARTITION_CODE_SEGMENT (space_id),
         addr, size, GDTE_CODE, 3);

   gdt_set_segment (GDT_PARTITION_DATA_SEGMENT (space_id),
         addr, size, GDTE_DATA, 3);
}

void ja_space_init(void)
{
    uintptr_t phys_start = POK_PARTITION_MEMORY_PHYS_START;
    for(int i = 0; i < ja_spaces_n; i++)
    {
        struct ja_x86_space* space = &ja_spaces[i];

        /* 
         * Code and data segments should be aligned on 4k;
         */
        size_t size_total = space->size_normal;

        if(space->size_heap > 0)
        {
            /* Heap should be aligned on 16; (why?) */
            size_total = ALIGN_VAL(size_total, 16) + space->size_heap;
        }

        // Store intermediate result.
        space->heap_end =  size_total;

        /* Stack should be aligned on 16; (why?) */
        size_total = ALIGN_VAL(size_total, 16) + space->size_stack;

        /* Such a way, next space will have alignment suitable for code and data. */
        space->size_total = ALIGN_VAL(size_total, 0x1000);

        if(space->phys_base == 0)
        {
            space->phys_base = ALIGN_VAL(phys_start, 0x1000);
            phys_start = space->phys_base + space->size_total;
        }
        else
        {
            assert(space->phys_base >= POK_PARTITION_MEMORY_PHYS_START);
            assert((space->phys_base & 0xfff) == 0);
            uintptr_t phys_base_end = space->phys_base + space->size_total;
            if(phys_start < phys_base_end)
                phys_start = phys_base_end;
        }
        ja_space_create(i + 1, (uintptr_t)space->phys_base, space->size_total);
    }
}

void ja_ustack_init (jet_space_id space_id)
{
    assert(space_id != 0);

    struct ja_x86_space* space = &ja_spaces[space_id - 1];

    space->size_stack_used = 0;
}

jet_ustack_t ja_ustack_alloc (jet_space_id space_id, size_t stack_size)
{
    assert(space_id != 0);

    struct ja_x86_space* space = &ja_spaces[space_id - 1];

    size_t size_stack_new = space->size_stack_used + ALIGN_VAL(stack_size, 16);

    if(size_stack_new > space->size_stack) return 0;

    jet_ustack_t result = POK_PARTITION_MEMORY_BASE + space->size_total - space->size_stack_used;

    space->size_stack_used = size_stack_new;

    return result;
}

void ja_space_switch (jet_space_id space_id)
{
    if(current_space_id != 0) {
        gdt_disable (GDT_PARTITION_CODE_SEGMENT(current_space_id));
        gdt_disable (GDT_PARTITION_DATA_SEGMENT(current_space_id));
    }
    if(space_id != 0) {
        gdt_enable (GDT_PARTITION_CODE_SEGMENT(space_id));
        gdt_enable (GDT_PARTITION_DATA_SEGMENT(space_id));
    }

    current_space_id = space_id;
}

jet_space_id ja_space_get_current (void)
{
    return current_space_id;
}

// TODO: Storage for floating point registers and operations with them.
struct jet_fp_store
{
  int todo;
};

/* 
 * Allocate place for store floating point registers.
 * 
 * May be called only during OS init.
 */
struct jet_fp_store* ja_alloc_fp_store(void)
{
    struct jet_fp_store* res = ja_mem_alloc_aligned(sizeof(*res), 4);

    return res;
}

/* Save floating point registers into given place. */
void ja_fp_save(struct jet_fp_store* fp_store)
{
    // TODO
    (void)fp_store;
}

/* Restore floating point registers into given place. */
void ja_fp_restore(struct jet_fp_store* fp_store)
{
    // TODO
    (void)fp_store;
}

/* Initialize floating point registers with zero. */
void ja_fp_init(void)
{
    // TODO
}


uintptr_t pok_virt_to_phys(uintptr_t virt)
{
    struct ja_x86_space* space = &ja_spaces[ja_space_get_current() - 1];

    if((virt < POK_PARTITION_MEMORY_BASE)
        || (virt > POK_PARTITION_MEMORY_BASE + space->size_total))
    {
        // Fatal error despite it is called from user space!!
        printf("pok_virt_to_phys: wrong virtual address %p\n", (void*)virt);
        pok_fatal("wrong pointer in pok_virt_to_phys\n");
    }

    return virt - POK_PARTITION_MEMORY_BASE + space->phys_base;
}

uintptr_t pok_phys_to_virt(uintptr_t phys)
{
    struct ja_x86_space* space = &ja_spaces[ja_space_get_current() - 1];

    if((phys < space->phys_base)
        || (phys >= space->phys_base + space->size_total))
    {
        // Fatal error despite it is called from user space!!
        printf("pok_phys_to_virt: wrong physical address %p\n", (void*)phys);
        pok_fatal("wrong pointer in pok_phys_to_virt\n");
    }

    return phys - space->phys_base + POK_PARTITION_MEMORY_BASE;
}
