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

/*
 * This file contains entry points to interrupt handlers.
 * They're called from the assembly contained in entry.S.
 *
 */

#include <types.h>
#include <errno.h> 
#include <core/debug.h>
#include <libc.h>
#include "reg.h"

#include "space.h"
#include "timer.h"
#include "syscalls.h"
#include "interrupt_context.h"



void pok_int_critical_input(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("Critical input interrupt"); 
}

void pok_int_ri(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("Unrealized instruction"); 
}

void pok_int_machine_check(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("Machine check interrupt"); 
}

void pok_int_data_storage(struct jet_interrupt_context *vctx, uintptr_t dear, unsigned long esr) {
    printf("TLB error\n");
    pok_arch_handle_page_fault(vctx, dear, esr, PF_DATA_STORAGE);
}

void pok_int_inst_storage(struct jet_interrupt_context* vctx, uintptr_t dear, unsigned long esr) {
    printf("TLB error\n");
    pok_arch_handle_page_fault(vctx, dear, esr, PF_INST_STORAGE);
}

void pok_int_ext_interrupt(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("External interrupt");
}

void pok_int_alignment(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("Alignment interrupt");
}

int k=0;

extern void * pok_trap_addr;
extern void * pok_trap;
void write_on_screen();


void pok_int_program(struct jet_interrupt_context* ea) {

//////printf("ea = 0x%lx\n", ea);
////// pok_trap_addr = address of pok_trap in entry.S
//#ifdef DEBUG_GDB
//    printf("    Pok_int_program interrupt\n");
//    int DBCR0 = mfspr(SPRN_DBCR0);
//    printf("DBCR0 = 0x%x\n", DBCR0);
//    printf("DBSR = %lx\n", mfspr(SPRN_DBSR));
//    printf("DAC1 = %lx\n", mfspr(SPRN_DAC1));
//    printf("DAC2 = %lx\n", mfspr(SPRN_DAC1));
//    printf("EPC = 0x%lx\n", ea->EPC);
//    printf("instr = 0x%lx\n", *(uint32_t *)ea->EPC);
//#endif
//
//    if (ea->EPC == (unsigned) (& pok_trap_addr)){
//        k++;
//#ifdef DEBUG_GDB
//        printf("Reason: SIGINT\n");
//#endif
//        handle_exception(17,ea); 
//    }else{
//#ifdef DEBUG_GDB
//        printf("Reason: Breakpoint\n");
//#endif
//        handle_exception(3,ea); 
//    }
//    //~ printf("\n\n            In pok_int_programm:\n");
//    //~ printf("addr = 0x%lx\n",(uint32_t) ea);
//    //~ printf("offset1 = 0x%lx\n",ea->offset1);
//    //~ printf("FCCR = 0x%lx\n",ea->FCCR);
//    //~ printf("CAUSE = 0x%lx\n",ea->CAUSE);
//    //~ printf("EPC or pc = 0x%lx\n",ea->EPC); 
//    //~ printf("STATUS = 0x%lx\n",ea->STATUS);
//    //~ printf("r0 = 0x%lx\n",ea->r0);
//    //~ printf("r1 = 0x%lx\n",ea->r1);
//    //~ printf("r2 = 0x%lx\n",ea->r2);
//    //~ printf("r3 = 0x%lx\n",ea->r3);
//    //~ printf("r4 = 0x%lx\n",ea->r4);
//    //~ printf("r5 = 0x%lx\n",ea->r5);
//    //~ printf("r6 = 0x%lx\n",ea->r6);
//    //~ printf("r7 = 0x%lx\n",ea->r7);
//    //~ printf("r8 = 0x%lx\n",ea->r8);
//    //~ printf("r9 = 0x%lx\n",ea->r9);
//    //~ printf("r10 = 0x%lx\n",ea->r10);
//    //~ printf("r11 = 0x%lx\n",ea->r11);
//    //~ printf("r12 = 0x%lx\n",ea->r12);
//    //~ printf("r13 = 0x%lx\n",ea->r13);
//    //~ printf("r14 = 0x%lx\n",ea->r14);
//    //~ printf("r15 = 0x%lx\n",ea->r15);
//    //~ printf("r16 = 0x%lx\n",ea->r16);
//    //~ printf("r17 = 0x%lx\n",ea->r17);
//    //~ printf("r18 = 0x%lx\n",ea->r18);
//    //~ printf("r19 = 0x%lx\n",ea->r19);
//    //~ printf("r20 = 0x%lx\n",ea->r20);
//    //~ printf("r21 = 0x%lx\n",ea->r21);
//    //~ printf("r22 = 0x%lx\n",ea->r22);
//    //~ printf("r23 = 0x%lx\n",ea->r23);
//    //~ printf("r24 = 0x%lx\n",ea->r24);
//    //~ printf("r25 = 0x%lx\n",ea->r25);
//    //~ printf("r26 = 0x%lx\n",ea->r26);
//    //~ printf("r27 = 0x%lx\n",ea->r27);
//    //~ printf("r28 = 0x%lx\n",ea->r28);
//    //~ printf("r29 = 0x%lx\n",ea->r29);
//    //~ printf("r30 = 0x%lx\n",ea->r30);
//    //~ printf("r31 = 0x%lx\n",ea->r31);
//
//
//    if (k == 1){
///*
// * it was a trap from gdb.c (in gdb.c function), probably we don't need it here.
// */ 
//        ea->EPC += 4;
//#ifdef DEBUG_GDB
//        printf("Change EPC");
//#endif
//    }
//    k=0;
//#ifdef DEBUG_GDB
//    printf("EPC = 0x%lx\n", ea->EPC);
//    printf("instr = 0x%lx\n", *(uint32_t *)ea->EPC);
//    //~ asm volatile("synci");
//    printf("instr = 0x%lx\n", *(uint32_t *)(ea->EPC));
//    DBCR0 = mfspr(SPRN_DBCR0);
//    printf("DBCR0 = 0x%x\n", DBCR0);
//    printf("\n          Exit from handle exception\n");
//#endif    
//////    pok_fatal("Program interrupt");
}

void pok_int_fp_unavail(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("FP unavailable interrupt");
}

void pok_int_none(struct jet_interrupt_context* ea) {
    (void) ea;
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    pok_fatal("Noknown interrupt :(");
}

void pok_int_overflow(struct jet_interrupt_context* ea) {
    (void) ea;
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    while (1 == 1){
    }
    pok_fatal("OVF Arithmetic overflow");
}

void pok_int_addrl(struct jet_interrupt_context* ea) {
    (void) ea;
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    pok_fatal("ADDRL Load from an illegal address");
}

void pok_int_addrs(struct jet_interrupt_context* ea) {
    (void) ea;
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    pok_fatal("ADDRS Store to an illegal address");
}

void pok_int_ibus(struct jet_interrupt_context* ea) {
    (void) ea;
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    pok_fatal("IBUS Bus error on instruction fetch");
}

void pok_int_dbus(struct jet_interrupt_context* ea) {
    (void) ea;
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    pok_fatal("DBUS Bus error on data reference");
}

unsigned long pok_int_system_call(struct jet_interrupt_context* ea,
    unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    printf("DEBUG: SYCALL!\n");
    printf("DEBUG: EPC   = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause = 0x%x\n", ea->CAUSE);
    (void) ea;
    return pok_arch_sc_int(arg1, arg2, arg3, arg4, arg5, arg6);
}

void pok_int_decrementer(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_arch_decr_int();
}

void pok_int_interval_timer(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("Inteval timer interrupt");
}

void pok_int_watchdog(struct jet_interrupt_context* ea) {
    (void) ea;
    pok_fatal("Watchdog interrupt");
}

void pok_int_data_tlb_miss(struct jet_interrupt_context* vctx, uintptr_t dear, unsigned long esr) {
    printf("TLB error\n");
    pok_arch_handle_page_fault(vctx, dear, esr, PF_DATA_TLB_MISS);
}

void pok_int_inst_tlb_miss(struct jet_interrupt_context* vctx, uintptr_t dear, unsigned long esr) {
    printf("TLB error\n");
    pok_arch_handle_page_fault(vctx, dear, esr, PF_INST_TLB_MISS);
}

void pok_int_debug(struct jet_interrupt_context* ea) {
    printf("DEBUG: EPC    = 0x%x\n", ea->EPC);
    printf("DEBUG: Cause  = 0x%x\n", ea->CAUSE);
    printf("DEBUG: Status = 0x%x\n", ea->STATUS);
    pok_fatal("BKPT break instruction executed");
}
