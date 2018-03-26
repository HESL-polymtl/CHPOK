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


#include <config.h>

#include <libc.h>
#include <asp/arch.h>
#include <core/partition_arinc.h>
#include <cons.h>


#ifdef POK_NEEDS_NETWORKING
static void pok_network_thread_init(void)
{
    pok_threads[NETWORK_THREAD].entry = pok_network_thread;
    pok_threads[NETWORK_THREAD].sp = pok_context_create(NETWORK_THREAD, 4096, (uintptr_t) pok_network_thread);
}
#endif


#define NCOMMANDS sizeof(commands)/sizeof(commands[0])



/*
 *  Don't use '[' in your command!
 */



/*
 * Convert string in int
 */

int atoi(char * s)
{
    int i, n;
    n = 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9'; ++i)
        n = 10 * n + (s[i] - '0');
    return n;
}

/* 
 * N-th bit means that N-th ARINC partition should paused when
 * monitor leaves active mode.
 */
static uint32_t partition_on_pause_bits = 0;

static void partition_pause_set(pok_partition_id_t id, uint8_t is_paused)
{
    assert(id < 32);
    
    uint32_t mask = 1 << id;
    
    if(is_paused)
        partition_on_pause_bits |= mask;
    else
        partition_on_pause_bits &= (~mask);
}
static uint8_t partition_pause_get(pok_partition_id_t id)
{
    assert(id < 32);
    
    return partition_on_pause_bits >> id & 1;
}

pok_bool_t want_to_exit=FALSE; 

int mon_help(int argc, char **argv);

int help_about(int argc,char **argv);

int print_partition(int argc, char **argv); // List of partition

int pause_N(int argc, char **argv);// pause partition N. 

int resume_N(int argc, char **argv); // continue partition N

int restart_N(int argc, char **argv); // restart pertition N

int exit_from_monitor(int argc, char **argv); //exit from monitor

int cpu_reset(int argc, char **argv); //cpu reset

int info_partition(int argc,char ** argv);

struct Command {
    const char *name;
    const char *argc;
    const char *desc;
    int (*func)(int argc, char** argv);
};

static struct Command commands[] = {
    { "help", "" , "Display all list of commands", mon_help },
    { "help_about" , "/command/" , "Display descriptions of this command", help_about},
    {"ps", "" ,"Display list of partition",print_partition},
    {"info_partition", "/N/" ,"Display information about partition N",info_partition},
    {"pause", "/N/" ,"Pause partition N",pause_N},
    {"resume", "/N/" ,"Continue partition N",resume_N},
    {"restart", "/N/" ,"Restart partition N",restart_N},
    {"reset", "" ,"reset cpu", cpu_reset},
    {"exit", "" ,"Exit from console",exit_from_monitor},
};

/* 
 * Implementations of monitor commands
 */

int
mon_help(int argc, char **argv)
{
    if (argc > 1){
        printf("Too many arguments for help!\n");
        return 0;
    }
    int i;
    (void) argc;
    (void) argv;
    for (i = 0; i < NCOMMANDS; i++)
        printf("%s %s- %s\n", commands[i].name,commands[i].argc,commands[i].desc);
    return 0;
}

int 
help_about(int argc,char **argv)
{
    if (argc > 2){
        printf("Too many arguments for help_about!\n");
        return 0;
    }
    if (argc == 1){
        printf("Missing parameter - Name of command!\n");
        return 0;
    }    
    for (int i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[1], commands[i].name) == 0){
            printf("%s\n",commands[i].desc);
            return 0;            
        }    
    }
    printf("There is no such command!\n");
    return 0;
}

int 
print_partition(int argc, char **argv){

    if (argc > 1){
        printf("Too many arguments for ps!\n");
        return 0;
    }

    if (pok_partitions_arinc_n > 1) 
        printf("There are %d partitions:\n", pok_partitions_arinc_n);
    else 
        printf("There is %d partition:\n", pok_partitions_arinc_n);
   
    for (int i = 0 ; i < pok_partitions_arinc_n ; i++){
        pok_partition_arinc_t* part = &pok_partitions_arinc[i];
        printf("Partition %d: %s", i, part->base_part.name);
        printf("\n");    
    }

    (void) argc;
    (void) argv;
    return 0;
}

int info_partition(int argc,char **argv){
    if (argc > 2){
        printf("Too many arguments for info_partition!\n");
        return 0;
    }
    if (argc == 1){
        printf("Missing parameter - Number of partition!\n");
        return 0;
    }    

    int number=0;
    number=atoi(argv[1]);
    if (number >= pok_partitions_arinc_n) {
        printf("There is no such partition!\n");
        return 0;        
    }
    
    pok_partition_arinc_t* part = &pok_partitions_arinc[number];
    struct jet_space_layout space_layout;
    ja_space_layout_get(part->base_part.space_id, &space_layout);
    
    printf("\n\n");
    printf("Info about partition #%d\n",number);
    printf("is_paused = %d\n", partition_pause_get(number));
    printf("base_addr = 0x%lx\n", (unsigned long)space_layout.kernel_addr);
    printf("base_vaddr = 0x%lx\n", (unsigned long)space_layout.user_addr);
    printf("size = 0x%zx\n", space_layout.size);
    printf("name = %s\n", part->base_part.name);
    printf("nthreads = %lu\n", (unsigned long)part->nthreads);
    printf("period = %lu\n", (unsigned long)part->base_part.period);
    printf("duration = %lu\n", (unsigned long)part->base_part.duration);
    //printf("activation = %llu\n", part->base_part.activation);
    printf("lock_level = %u\n", (unsigned) part->lock_level);
    printf("prev_thread = %u\n", part->lock_level ? (unsigned)(part->thread_locked - part->threads) : (unsigned)-1);
    printf("current_thread = %u\n", (unsigned)(part->thread_current - part->threads));
    printf("thread_main = %u\n", POK_PARTITION_ARINC_MAIN_THREAD_ID);
#ifdef POK_NEEDS_IO
    //printf("io_min = %d\n",pok_partitions[number].io_min);        
    //printf("io_max = %d\n",pok_partitions[number].io_max);        
#endif
    printf("\n\n");

    return 0;
}

int 
pause_N(int argc, char **argv){
    if (argc > 2){
        printf("Too many arguments for pause!\n");
        return 0;
    }
    if (argc == 1){
        printf("Missing parameter - Number of partition!\n");
        return 0;
    }    
    int number=0;
    number=atoi(argv[1]);
    if (number >= pok_partitions_arinc_n) {
        printf("There is no such partition!\n");
        return 0;        
    }
    //Change mode of this partition to paused
    partition_pause_set(number, 1);
    printf("Partition %d paused\n", number);

    return 0;
}

int 
resume_N(int argc, char **argv){

    if (argc > 2){
        printf("Too many arguments for resume!\n");
        return 0;
    }
    if (argc == 1){
        printf("Missing parameter - Number of partition!\n");
        return 0;
    }    

    int number=0;
    number=atoi(argv[1]);
    if (number >= pok_partitions_arinc_n) {
        printf("There is no such partition!\n");
        return 0;        
    }
    //Change mode of this partition to not paused
    partition_pause_set(number, 0);
    printf("Partition %d resumed\n",number);
    return 0;
}

int 
restart_N(int argc, char **argv){

    if (argc > 2){
        printf("Too many arguments for restart!\n");
        return 0;
    }
    if (argc == 1){
        printf("Missing parameter - Number of partition!\n");
        return 0;
    }    

    int number=0;
    number=atoi(argv[1]);
    if (number >= pok_partitions_arinc_n) {
        printf("There is no such partition!\n");
        return 0;        
    }

    // Restart partition via global scheduler.
    pok_partitions_arinc[number].base_part.sp = 0;

    printf("Partition %d restarted\n",number);

    return 0;
}

int 
exit_from_monitor(int argc, char **argv){

    (void) argc;
    (void) argv;
    want_to_exit=TRUE;
    return 0;

}

int cpu_reset(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    printf("Rebooting\n");
    ja_cpu_reset();

    return 0;
}



/*
 * Monitor command interpreter
 */

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf) 
{
    int argc;
    char *argv[MAXARGS];
    int i;

    // Parse the command buffer into whitespace-separated arguments
    argc = 0;
    argv[argc] = 0;
    while (1) {
        // gobble whitespace
        while (*buf && strchr(WHITESPACE, *buf))
            *buf++ = 0;
        if (*buf == 0)
            break;

        // save and scan past next arg
        if (argc == MAXARGS-1) {
            printf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf))
            buf++;
    }
    argv[argc] = 0;

    // Lookup and invoke the command
    if (argc == 0)
        return 0;
    for (i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv);
    }
    printf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void
monitor()
{
    char *buf;
    want_to_exit=FALSE;
    printf("Welcome to the monitor!\n");
    printf("Type 'help' for a list of commands.\n");


    while (!want_to_exit) {
        buf = readline("K> ");
        if (buf != NULL)
            if (runcmd(buf) < 0)
                break;
            
    }
}


void monitor_start_func(void)
{
    for (int i=0; i < pok_partitions_arinc_n; i++){
        partition_pause_set(i, 1);
    }
    for (;;) {
        if (data_to_read_0() == 1) {
            /*
             * Set all partitions on pause
             */
            pok_preemption_disable();
            for (int i=0; i < pok_partitions_arinc_n; i++){
                pok_partition_arinc_t* part = &pok_partitions_arinc[i];
                if (!part->base_part.is_paused){ 
                    partition_pause_set(i, 0);
                    part->base_part.is_paused=TRUE;
                }
            }
            pok_preemption_enable();
            
            monitor();

            pok_preemption_disable();
            for (int i=0; i < pok_partitions_arinc_n; i++){
                if (!partition_pause_get(i)){ 
                    pok_partitions_arinc[i].base_part.is_paused=FALSE;
                }
            }
            pok_preemption_enable();
        }
    }
}

void monitor_process_error(pok_system_state_t partition_state,
        pok_error_id_t error_id,
        uint8_t state_byte_preempt_local,
        void* failed_address)
{
    pok_fatal("Error in monitor");
}

static const struct pok_partition_operations monitor_operations =
{
    .start = monitor_start_func,
    .process_partition_error = monitor_process_error
};

void pok_monitor_thread_init()
{
#ifdef POK_NEEDS_MONITOR
    pok_partition_init(&partition_monitor);

    partition_monitor.part_sched_ops = &partition_sched_ops_kernel;
    partition_monitor.part_ops = &monitor_operations;
    partition_monitor.initial_sp = pok_stack_alloc(4096);
#endif
}

