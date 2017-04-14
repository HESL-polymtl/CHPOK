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

#include <types.h>
#include <libc.h>
#include <core/error.h>
#include <core/partition.h>
#include <elf.h>

#include <core/loader.h>

#include <core/memblocks.h>
#include <core/uaccess.h>

static const struct memory_block* get_memory_block_for_addr(
    const struct memory_block* const* mblocks,
    void* __user addr,
    size_t size
    )
{
    uintptr_t vaddr = (uintptr_t)addr;

    for(const struct memory_block* const* p_mblock = mblocks;
        *p_mblock != NULL;
        p_mblock++)
    {
        const struct memory_block* mblock = *p_mblock;

        if(mblock->vaddr > vaddr) break;
        if(mblock->vaddr + mblock->size > vaddr) {
            // Maximum size available to the end of the block.
            uint64_t rest_size = mblock->vaddr + mblock->size - vaddr;

            // Check that end of range is not after the end of the block.
            if(size > rest_size) return NULL;

            return mblock;
        }

    }

    return NULL;
}

void jet_loader_elf_load   (const void* elf_image,
                                 pok_partition_arinc_t* part,
                                 const struct memory_block* const* mblocks,
                                 void (** entry)(void))
{
    const Elf32_Ehdr*  elf_header;
    const Elf32_Phdr*  elf_phdr;

    elf_header = (Elf32_Ehdr*)elf_image;

    if (elf_header->e_ident[0] != 0x7f ||
         elf_header->e_ident[1] != 'E' ||
         elf_header->e_ident[2] != 'L' ||
         elf_header->e_ident[3] != 'F')
    {
        printf("Partition's ELF has incorrect format");
        pok_raise_error(POK_ERROR_ID_PARTLOAD_ERROR, FALSE, NULL);
    }

    *entry = (void (*)(void)) elf_header->e_entry;

    elf_phdr = (Elf32_Phdr*)((const char*)elf_image + elf_header->e_phoff);

    // Iterate over ELF segments.
    for (int i = 0; i < elf_header->e_phnum; ++i)
    {
        char* __user vstart = (char * __user)elf_phdr[i].p_vaddr;
        size_t memsz = elf_phdr[i].p_memsz;

        if(memsz == 0) continue; // Skip zero-length segments.

        const struct memory_block* mblock = get_memory_block_for_addr(
            mblocks, vstart, memsz);

        if(mblock == NULL) {
            printf("ELF segment %d of partition '%s' isn't contained in any memory block.\n",
                i, part->base_part.name);
            pok_raise_error(POK_ERROR_ID_PARTLOAD_ERROR, FALSE, NULL);
        }

        char* __kuser kstart = jet_memory_block_get_kaddr(mblock, vstart);

        size_t filesz = elf_phdr[i].p_filesz;

        if(filesz) {
            memcpy(kstart, (const char*)elf_image + elf_phdr[i].p_offset,
                filesz);
        }
        if(memsz > filesz) {
            memset(kstart + filesz, 0, memsz - filesz);
        }
    }
}
