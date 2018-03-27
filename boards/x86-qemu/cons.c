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
#include <asp/cons.h>

#include "cons.h"

#ifdef POK_NEEDS_CONSOLE

#define  COM0      0x3F8
#define  COM1      0x2f8

struct s_cons
{
  char row;
  char col;
  char cur_attr;
};

static const int     screen_w = 80;
static const int     screen_h = 25;
static const int     tab_size = 8;
static char *const   vga_base = (char *)0xb8000;
struct s_cons        g_cons;


int is_transmit_empty(int port) {
   return inb(port + 5) & 0x20;
}

void write_serial(int port, char a) {
   while (is_transmit_empty(port) == 0);

   outb(port, a);
}

#define COM_LSR		5	// In:	Line Status Register
#define COM_RX		0	// In:	Receive buffer (DLAB=0)
#define   COM_LSR_DATA	0x01	//   Data available
#define   COM_LSR_RFE	0x80	//   Error in Received FIFO

static int data_to_read(int port) //return 0 if no data to read
{
    int flags = inb(port + COM_LSR);
	if (!(flags & COM_LSR_DATA) || (flags & COM_LSR_RFE))
		return 0;
	return 1;
}

static int read_serial(int port)
{
	if(!data_to_read(port)) return -1;

   int data;
	data=inb(port+COM_RX);
	if ( !(inb(port+COM_LSR) & COM_LSR_RFE) )
		return data;
	return -1;
}

static size_t iostream_read_common(int port, char* s, size_t length)
{
   size_t i;

   for(i = 0; i != length; i++)
   {
      int data = read_serial(port);
      if(data == -1) break;

      s[i] = (char)data;
   }

   return i;
}

void pok_cons_clear (void)
{
   int   i;
   int   j;
   char  *ptr;
   static struct s_cons		local_curs; /* local copy of the current cursor position */

   ptr = vga_base;
   local_curs = g_cons;

   for (i = 0 ; i < screen_h ; ++i)
   {
      for (j = 0 ; j < screen_w ; ++j)
      {
         *ptr = 0;
         ++ptr;
         *ptr = g_cons.cur_attr;
         ++ptr;
      }
   }

   local_curs.row = 0;
   local_curs.col = 0;

   g_cons = local_curs; /* reset the global cursor to the new position */
}

static void pok_write_vga (const char c)
{
   char*                ptr;
   static struct s_cons local_curs; /* Local copy of the curent cursor position */
   int                  i;

   local_curs = g_cons;

   if (c == '\r')
   {
      local_curs.col = 0;
   }

   if (c == '\n')
   {
      local_curs.col = 0;
      ++local_curs.row;
   }

   if (c == '\t')
   {
      local_curs.col += tab_size - local_curs.col % tab_size;
   }

   if (c != '\r' && c != '\n' && c != '\t')
   {
      ptr = vga_base + 2 * screen_w * local_curs.row + 2 * local_curs.col++;
      *ptr = c;
      ++ptr;
      *ptr = local_curs.cur_attr;
   }

   if (local_curs.col >= screen_w)
   {
      local_curs.col = 0;
      ++local_curs.row;
   }

   if (local_curs.row >= screen_h)
   {
      memcpy (vga_base, vga_base + 2 * screen_w, (screen_h - 1) * screen_w * 2);
      for (i = 0; i < screen_w; ++i)
      {
         *(vga_base + 2 * screen_w * (screen_h - 1) + 2 * i) = 0;
         *(vga_base + 2 * screen_w * (screen_h - 1) + 2 * i + 1) = local_curs.cur_attr;
      }
      local_curs.row = screen_h - 1;
   }

   // Reset the global cursor to the new position
   g_cons = local_curs;

}

static size_t iostream_write_common (int port, const char *s, size_t length)
{
    char c;
    size_t rest = length;
    for (; rest > 0; rest--) {
        c = *s++;
        if (c != '\n')
            write_serial(port, c);
        else {
            write_serial(port, '\r');
            write_serial(port, '\n');
        }
	pok_write_vga(c);
    }




   return length;
}

static void iostream_init_common (int port)
{
   /* Init VGA screen */
   pok_cons_clear ();
   g_cons.cur_attr = 0x7 | 0 << 4;
   /* To be fixed : init serial */
   outb(port + 1, 0x00);    // Disable all interrupts
   outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(port + 1, 0x00);    //                  (hi byte)
   outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static size_t iostream_read_main(char* s, size_t length)
{
    return iostream_read_common(COM0, s, length);
}
static size_t iostream_write_main(const char* s, size_t length)
{
   return iostream_write_common(COM0, s, length);
}
static void iostream_init_main(void)
{
   iostream_init_common(COM0);
}

static struct jet_iostream x86_stream_main =
{
    .write = &iostream_write_main,
    .read  = &iostream_read_main,
    .init = &iostream_init_main
};


static size_t iostream_read_debug(char* s, size_t length)
{
    return iostream_read_common(COM1, s, length);
}
static size_t iostream_write_debug(const char* s, size_t length)
{
   return iostream_write_common(COM1, s, length);
}
static void iostream_init_debug(void)
{
   iostream_init_common(COM1);
}

static struct jet_iostream x86_stream_debug =
{
    .write = &iostream_write_debug,
    .read  = &iostream_read_debug,
    .init = &iostream_init_debug
};

struct jet_iostream* ja_stream_default_read = &x86_stream_main;
struct jet_iostream* ja_stream_default_write = &x86_stream_main;
struct jet_iostream* ja_stream_default_read_debug = &x86_stream_debug;
struct jet_iostream* ja_stream_default_write_debug = &x86_stream_debug;

#else

struct jet_iostream* ja_stream_default_read = NULL;
struct jet_iostream* ja_stream_default_write = NULL;
struct jet_iostream* ja_stream_default_read_debug = NULL;
struct jet_iostream* ja_stream_default_write_debug = NULL;

#endif
