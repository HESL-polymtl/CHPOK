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

#ifndef __JET_PPC_FP_REGISTERS_H__
#define __JET_PPC_FP_REGISTERS_H__

#include <asp/space.h>

struct jet_fp_store
{
  uint64_t fp_regs[32];
  uint64_t fp_fpscr;
};

#endif /* __JET_PPC_FP_REGISTERS_H__ */
