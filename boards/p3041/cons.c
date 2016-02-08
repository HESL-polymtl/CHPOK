/*
 *                               POK header
 * 
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team 
 *
 * Created by julien on Thu Jan 15 23:34:13 2009 
 */

#include <config.h>

#include <errno.h>

#include <ioports.h>
#include <libc.h>
#include <core/debug.h>
#include <core/cons.h>
#include <cons.h>
#include <bsp.h>

struct port_buf ports_stack[2];

#if defined (POK_NEEDS_CONSOLE) || defined (POK_NEEDS_DEBUG) || defined (POK_NEEDS_INSTRUMENTATION) || defined (POK_NEEDS_COVERAGE_INFOS)

#define NS16550_REG_THR 0
#define NS16550_REG_LSR 5

#define UART_LSR_THRE   0x20

pok_bsp_t pok_bsp = {
    .ccsrbar_size = 0x1000000ULL,
    .ccsrbar_base = 0x0FE000000ULL,
    .ccsrbar_base_phys = 0x0FE000000ULL,
    .serial0_regs_offset = 0x11C500ULL,
    .timebase_freq = 17000000,
    .serial1_regs_offset = 0x11C600ULL
};

static void ns16550_writeb(int offset, int value, int flag)
{
    if (flag == 0){
        outb(pok_bsp.ccsrbar_base + pok_bsp.serial0_regs_offset + offset, value);
    } else {
        outb(pok_bsp.ccsrbar_base + pok_bsp.serial1_regs_offset + offset, value);
    }
}

static int ns16550_readb(int offset, int flag)
{
    if (flag == 0) {
        return inb(pok_bsp.ccsrbar_base + pok_bsp.serial0_regs_offset + offset);
    }
    return inb(pok_bsp.ccsrbar_base + pok_bsp.serial1_regs_offset + offset);
}

void     read_all_from_port();


 void write_serial_1(char a)
{
    //~ read_all_from_port();
   while ((ns16550_readb(NS16550_REG_LSR, 0) & UART_LSR_THRE) == 0)
     ;
    //~ printf("After While read_all\n");


   ns16550_writeb(NS16550_REG_THR, 127 + 1, 0);
    //~ printf("After 2  While read_all\n");
   ns16550_writeb(NS16550_REG_THR, a, 0);

   //~ ns16550_writeb(NS16550_REG_THR, a, 1);
}

static void write_serial_0(char a)
{
   while ((ns16550_readb(NS16550_REG_LSR, 0) & UART_LSR_THRE) == 0)
     ;

   ns16550_writeb(NS16550_REG_THR, 127 + 0, 0);


   ns16550_writeb(NS16550_REG_THR, a, 0);
    //write_serial_1(a);
}

#define UART_LSR_DR   0x01
#define UART_LSR_RFE  0x80


int data_to_read_from_port() //return 0 if no data to read
{
    int flags = ns16550_readb(NS16550_REG_LSR, 0);
    if ((!(flags & UART_LSR_DR)) || (flags & UART_LSR_RFE))
        {
        //~ printf("No!");
        return 0;
    }
        //~ printf("Yes!");
    return 1;
}

    
int read_serial_from_port()
{
    //~ printf("read_serial_from_port! \n");
    int port,data;
    port = ns16550_readb(NS16550_REG_THR,0);
    //~ printf("port = %d\n",port);
    if ((ns16550_readb(NS16550_REG_LSR,0) & UART_LSR_RFE) )
        return -1;
    while (port < 127) {
        while (data_to_read_from_port() == 0){
            }
        port = ns16550_readb(NS16550_REG_THR,0);
        if ((ns16550_readb(NS16550_REG_LSR,0) & UART_LSR_RFE) )
            return -1;        
    }
    port = port - 127; //127 = max number of char in ASCII
    //~ printf("Port: value = %d('%c'), waiting for next symbol...\n", port, port + 127);
    //~ printf("Port = %c \n",port);
    
    //~ if (port == 'a'){
        //~ port = 1;
    //~ } else  if (port == 'b'){
        //~ port = 1;
        //~ for (int i = (ports_stack[port].last_read + 1) % BUF_MAX_SIZE; i != (ports_stack[port].last_write + 1) % BUF_MAX_SIZE; i = (i + 1) % BUF_MAX_SIZE)
            //~ printf("%c_",ports_stack[port].data[i]);
        //~ printf("\n");
    //~ } else     port = 0;

    while (data_to_read_from_port() == 0){
    }
    data = ns16550_readb(NS16550_REG_THR,0);
    if ( (ns16550_readb(NS16550_REG_LSR,0) & UART_LSR_RFE) )
        return -1;
    ports_stack[port].last_write = (ports_stack[port].last_write + 1) % BUF_MAX_SIZE;
    if (ports_stack[port].last_read == ports_stack[port].last_write) //if there is stack overflow
        printf("WARNING : stack overflow of port = %d",port);
    ports_stack[port].data[ports_stack[port].last_write] = data;
    //~ printf("Read new symbol '%c',%d: last_read = %d, last_write %d\n", data, data, ports_stack[port].last_read, ports_stack[port].last_write);
    return 0;
}

void read_all_from_port()
{
    int error;
    while (data_to_read_from_port() == 1){
        error = read_serial_from_port();
        if (error != 0) printf("ERROR : in read from port\n");
    }
}

#define LCR 0x3

void return_DLB(){
    int old_LCR = ns16550_readb(LCR, 0);
    int new_LCR = old_LCR | 0x80;
    ns16550_writeb(LCR, new_LCR, 0);
    int LB_1 = 0xa4;
    int MB_1 = 0x00;
    ns16550_writeb(NS16550_REG_THR, LB_1, 0);
    ns16550_writeb(NS16550_REG_THR + 0x1, MB_1, 0);
    

    int LB = ns16550_readb(NS16550_REG_THR, 0);
    int MB = ns16550_readb(NS16550_REG_THR + 0x1, 0);
    ns16550_writeb(LCR, old_LCR, 0);

    int j = 0;
    int symbol = -1;
    while (1){
        while (!data_to_read_0()){
            int l=0;
            for (int k = 0; k < 1000000; k ++){
                l++;
            }
        }
        symbol = read_serial_0();
        if (symbol != j){
            printf("ERROR: j = %d, symbol = %d!!!\n", j, symbol);
            while (1){
            }
        }
        j = (j + 1) % 256;
    }
    
    
    printf("=====\n");
    for (int i = 0; i<= 10000; i++){
        printf("#");
    }
    printf("=====\n");
    
    printf("LB = %x , MB = %x, \n", LB, MB); 

}



int data_to_read_0() //return 0 if no data to read
{
    //~ read_all_from_port();
    if (ports_stack[0].last_read == ports_stack[0].last_write)
        return 0;
    return 1;
}

int data_to_read_1() //return 0 if no data to read
          
{
    //~ int flags = ns16550_readb(NS16550_REG_LSR, 1);
    //~ if ((!(flags & UART_LSR_DR)) || (flags & UART_LSR_RFE))
        //~ return 0;
    //~ return 1;
    
    read_all_from_port();
    if (ports_stack[1].last_read == ports_stack[1].last_write)
        return 0;
    return 1;
}

int read_serial_0()
{
    int data;
    //~ int ports = 0;
//~ 
    //~ for (int i = (ports_stack[ports].last_read + 1) % BUF_MAX_SIZE; i != (ports_stack[ports].last_write + 1) % BUF_MAX_SIZE; i = (i + 1) % BUF_MAX_SIZE)
        //~ printf("%c,%d_",ports_stack[ports].data[i],ports_stack[ports].data[i]);
    //~ printf("\n");
    //~ printf("Ports: last_read = %d; last_write = %d\n", ports_stack[ports].last_read, ports_stack[ports].last_write);
    //~ ports ++;
    //~ for (int i = (ports_stack[ports].last_read + 1) % BUF_MAX_SIZE; i != (ports_stack[ports].last_write + 1) % BUF_MAX_SIZE; i = (i + 1) % BUF_MAX_SIZE)
        //~ printf("%c,%d_",ports_stack[ports].data[i],ports_stack[ports].data[i]);
    //~ printf("\n");
    ports_stack[0].last_read = (ports_stack[0].last_read + 1) % BUF_MAX_SIZE;
    data = ports_stack[0].data[ports_stack[0].last_read];


    //~ printf("read_serial: Data = '%c'; \n", data);
    return data;
}

int read_serial_1()
{
    //~ int data;
    //~ data=ns16550_readb(NS16550_REG_THR,1);
    //~ if ( !(ns16550_readb(NS16550_REG_LSR,1) & UART_LSR_RFE) )
        //~ return data;
    //~ return -1;
    int data;
    ports_stack[1].last_read = (ports_stack[1].last_read + 1) % BUF_MAX_SIZE;
    data = ports_stack[1].data[ports_stack[1].last_read];
    return data;

}


pok_bool_t pok_cons_write (const char *s, size_t length)
{
    //~ read_all_from_port();
    char c;
    for (; length > 0; length--) {
        c = *s++;
        if (c != '\n')
            write_serial_0(c);
        else {
            write_serial_0('\r');
            write_serial_0('\n');
        }
    }
   return 0;
}

pok_bool_t pok_cons_write_1 (const char *s, size_t length)
{
    char c;
    for (; length > 0; length--) {
        c = *s++;
        if (c != '\n')
            write_serial_1(c);
        else {
            write_serial_1('\r');
            write_serial_1('\n');
        }
    }
   return 0;
}

int pok_cons_init (void)
{
    //~ return_DLB();
    pok_print_init (write_serial_0, NULL);
    return 0;
}
#else
int pok_cons_init (void)
{
   return 0;
}
#endif