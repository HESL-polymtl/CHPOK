/*
 * GENERATED! DO NOT MODIFY!
 *
 * Instead of modifying this file, modify the one it generated from (kernel/arch/x86/exceptions/exceptions.yaml).
 */
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

#include <interrupt.h>
#include <core/error.h>
#include <event.h>


#include <bsp/bsp.h>

// Declare exception functions. They are defined in exception_entries.S
void exception_DIVIDE_ERROR(void);
void exception_DEBUG(void);
void exception_NMI(void);
void exception_BREAKPOINT(void);
void exception_OVERFLOW(void);
void exception_INVALIDOPCODE(void);
void exception_NOMATH_COPROC(void);
void exception_DOUBLEFAULT(void);
void exception_COPSEG_OVERRUN(void);
void exception_INVALID_TSS(void);
void exception_SEGMENT_NOT_PRESENT(void);
void exception_STACKSEG_FAULT(void);
void exception_GENERAL_PROTECTION(void);
void exception_PAGEFAULT(void);
void exception_FPU_FAULT(void);
void exception_ALIGNEMENT_CHECK(void);
void exception_MACHINE_CHECK(void);
void exception_SIMD_FAULT(void);
void exception_SYSCALL(void);
void exception_TIMER(void);


const struct exception_descriptor exception_list[] =
{
    {EXCEPTION_DIVIDE_ERROR, exception_DIVIDE_ERROR},
    {EXCEPTION_DEBUG, exception_DEBUG},
    {EXCEPTION_NMI, exception_NMI},
    {EXCEPTION_BREAKPOINT, exception_BREAKPOINT},
    {EXCEPTION_OVERFLOW, exception_OVERFLOW},
    {EXCEPTION_INVALIDOPCODE, exception_INVALIDOPCODE},
    {EXCEPTION_NOMATH_COPROC, exception_NOMATH_COPROC},
    {EXCEPTION_DOUBLEFAULT, exception_DOUBLEFAULT},
    {EXCEPTION_COPSEG_OVERRUN, exception_COPSEG_OVERRUN},
    {EXCEPTION_INVALID_TSS, exception_INVALID_TSS},
    {EXCEPTION_SEGMENT_NOT_PRESENT, exception_SEGMENT_NOT_PRESENT},
    {EXCEPTION_STACKSEG_FAULT, exception_STACKSEG_FAULT},
    {EXCEPTION_GENERAL_PROTECTION, exception_GENERAL_PROTECTION},
    {EXCEPTION_PAGEFAULT, exception_PAGEFAULT},
    {EXCEPTION_FPU_FAULT, exception_FPU_FAULT},
    {EXCEPTION_ALIGNEMENT_CHECK, exception_ALIGNEMENT_CHECK},
    {EXCEPTION_MACHINE_CHECK, exception_MACHINE_CHECK},
    {EXCEPTION_SIMD_FAULT, exception_SIMD_FAULT},
    {EXCEPTION_SYSCALL, exception_SYSCALL},
    {EXCEPTION_TIMER, exception_TIMER},
    {0, NULL}
};

void ja_exception_init(void)
{
  int i;

  for (i = 0; exception_list[i].handler != NULL; ++i)
  {
    pok_idt_set_gate (exception_list[i].vector,
                      exception_list[i].handler,
                      IDTE_INTERRUPT);
  }
}


__attribute__((unused))
static void dump_registers (interrupt_frame *frame)
{
  printf ("ES: %lx, DS: %lx\n",  (unsigned long)frame->es, (unsigned long)frame->ds);
  printf ("CS: %lx, SS: %lx\n",  (unsigned long)frame->cs, (unsigned long)frame->ss);
  printf ("EDI: %lx, ESI: %lx\n", (unsigned long)frame->edi, (unsigned long)frame->esi);
  printf ("EBP: %lx, ESP: %lx\n", (unsigned long)frame->ebp, (unsigned long)frame->esp);
  printf ("EAX: %lx, ECX: %lx\n", (unsigned long)frame->eax, (unsigned long)frame->ecx);
  printf ("EDX: %lx, EBX: %lx\n", (unsigned long)frame->edx, (unsigned long)frame->ebx);
  printf ("EIP: %lx, ErrorCode: %lx\n", (unsigned long)frame->eip, (unsigned long)frame->error);
  printf ("EFLAGS: %lx\n\n", (unsigned long)frame->eflags);
}

/* 
 * Raise error from interrupt handler.
 * 
 * Call pok_raise_error() with parameters error_id, user_space,
 * failed_addr passed to it and output formatted message, given as rest
 * parameters, with printf_debug().
 * 
 * Interrupt frame is ignored in any case.
 */
#define raise_error_from_interrupt(error_id, user_space, failed_addr, ... /* message format and its args */) \
   do { (void)frame; printf_debug(__VA_ARGS__); \
   pok_raise_error(error_id, user_space, failed_addr); } while(0)

/* Helper. dump_registers() in debug mode, otherwise argument is ignored. */
#ifdef POK_NEEDS_DEBUG
#define dump_registers_debug(frame) dump_registers(frame)
#else
#define dump_registers_debug(frame) (void)frame
#endif

/* 
 * Raise error from interrupt handler and dump registers.
 * 
 * Call pok_raise_error() with parameters error_id, user_space,
 * failed_addr passed to it and output formatted message, given as rest
 * parameters, with printf_debug().
 * 
 * If debugging is enabled then dump_registers() is called.
 */
#define raise_error_from_interrupt_dump(error_id, user_space, failed_addr, ... /* message format and its args */) \
   do { dump_registers_debug(frame); printf_debug(__VA_ARGS__); \
   pok_raise_error(error_id, user_space, failed_addr); } while(0)

// Define exception handlers. They are called in exception_entries.S.
void exception_DIVIDE_ERROR_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt(POK_ERROR_ID_NUMERIC_ERROR,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise divide by zero error\n"    );
}
void exception_DEBUG_handler(interrupt_frame* frame)
{
    handle_exception(3, frame);
}
void exception_NMI_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_ILLEGAL_REQUEST,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception NMI fault\n"    );
}
void exception_BREAKPOINT_handler(interrupt_frame* frame)
{
    process_breakpoint(frame);
}
void exception_OVERFLOW_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_STACK_OVERFLOW,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception bound range fault\n"    );
}
void exception_INVALIDOPCODE_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_ILLEGAL_REQUEST,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception invalid opcode fault, EIP: 0x%lx\n", (unsigned long)frame->eip
    );
}
void exception_NOMATH_COPROC_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_ILLEGAL_REQUEST,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception no math coprocessor fault\n"    );
}
void exception_DOUBLEFAULT_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_UNHANDLED_INT,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception double fault\n"    );
}
void exception_COPSEG_OVERRUN_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_MEMORY_VIOLATION,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception copseg overrun fault\n"    );
}
void exception_INVALID_TSS_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_MEMORY_VIOLATION,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception invalid tss fault\n"    );
}
void exception_SEGMENT_NOT_PRESENT_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_MEMORY_VIOLATION,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception segment not present fault\n"    );
}
void exception_STACKSEG_FAULT_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_MEMORY_VIOLATION,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception stack segment fault\n"    );
}
void exception_GENERAL_PROTECTION_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_ILLEGAL_REQUEST,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception general protection fault. EIP=0x%lx\n", (unsigned long)frame->eip
    );
}
void exception_PAGEFAULT_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_MEMORY_VIOLATION,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception pagefault\n"    );
}
void exception_FPU_FAULT_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_HARDWARE_FAULT,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception FPU fault\n"    );
}
void exception_ALIGNEMENT_CHECK_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_HARDWARE_FAULT,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception alignment fault\n"    );
}
void exception_MACHINE_CHECK_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_HARDWARE_FAULT,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception machine check fault\n"    );
}
void exception_SIMD_FAULT_handler(interrupt_frame* frame)
{
    raise_error_from_interrupt_dump(POK_ERROR_ID_HARDWARE_FAULT,
      /*TODO: User or kernel*/TRUE,
      /*TODO: Failed address*/NULL,
      "[KERNEL] Raise exception SIMD fault\n"    );
}
void exception_SYSCALL_handler(interrupt_frame* frame)
{
    process_syscall(frame);
}
void exception_TIMER_handler(interrupt_frame* frame)
{
    ja_bsp_process_timer(frame);
}
