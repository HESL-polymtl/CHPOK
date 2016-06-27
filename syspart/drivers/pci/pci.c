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

#include <pci.h>
#include <ioports.h>
#include <stdio.h>

#include <bsp.h>
#include "pci_internal.h"
#include <depl.h>

#include <libc/string.h>

#define OK 0
unsigned pci_driver_table_used_cnt = 0;

#ifdef __PPC__
struct pci_bridge bridge;
#endif

static char *get_pci_class_name(int classcode) {
    int i = 0;
    while (pci_classnames[i].name != NULL) {
        if (pci_classnames[i].classcode == (classcode & 0xFFFF00))
            return pci_classnames[i].name;
        i++;
    }
    return "Unknown";
}



int pci_match_device(const struct pci_dev_id *id, const struct pci_dev *dev)
{
    if ((id->vendor == PCI_ANY_ID || id->vendor == dev->vendor_id) &&
            (id->device == PCI_ANY_ID || id->device == dev->device_id))
        return 1;
    return 0;
}

void register_pci_driver(struct pci_driver *driver)
{
    pci_driver_table[pci_driver_table_used_cnt++] = *driver;
}

//Depricated
uint32_t pci_read(
        uint32_t bus,
        uint32_t dev,
        uint32_t fn,
        uint32_t reg)
{
    uint32_t addr = (1 << 31) | (bus << 16) | (dev << 11) | 
        (fn << 8) | (reg & 0xfc);
    uint32_t val = -1;

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    val = in_le32((uint32_t *) bridge.cfg_data);
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    val = inl(PCI_CONFIG_DATA);
#endif

    return (val >> ((reg & 3) << 3));
}

void pci_write_word(s_pci_device *d, uint32_t reg, uint16_t val)
{
    uint32_t addr = (1 << 31) | (d->bus << 16) | (d->dev << 11) | 
        (d->fn << 8) | (reg & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    out_le16((uint16_t *) bridge.cfg_data, val);
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    outw(PCI_CONFIG_DATA, val);
#endif

}
void pci_write_dword(s_pci_device *d, uint32_t reg, uint32_t val)
{
    uint32_t addr = (1 << 31) | (d->bus << 16) | (d->dev << 11) | 
        (d->fn << 8) | (reg & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    out_le32((uint32_t *) bridge.cfg_data, val);
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    outw(PCI_CONFIG_DATA, val);
#endif

}


int pci_read_config_byte(struct pci_dev *dev, int where, uint8_t *val)
{
    uint32_t addr = (uint32_t) (1 << 31) | (dev->bus << 16) | (dev->dev << 11) |
        (dev->fn << 8) | (where & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    *val = in_8((void *) (bridge.cfg_data) + (where & 3));
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    *val = inb(PCI_CONFIG_DATA + (where & 3));
#endif
    return OK;
}

int pci_read_config_word(struct pci_dev *dev, int where, uint16_t *val)
{
    uint32_t addr = (uint32_t) (1 << 31) | (dev->bus << 16) | (dev->dev << 11) |
        (dev->fn << 8) | (where & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    *val = in_le16((void *) (bridge.cfg_data) + (where & 3));
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    *val = inw(PCI_CONFIG_DATA + (where & 2));
#endif
    return OK;
}

int pci_read_config_dword(struct pci_dev *dev, int where, uint32_t *val)
{
    uint32_t addr = (uint32_t) (1 << 31) | (dev->bus << 16) | (dev->dev << 11) |
        (dev->fn << 8) | (where & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    *val = in_le32((void *) bridge.cfg_data + (where & 3));
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    *val = inl(PCI_CONFIG_DATA);
#endif
    return OK;
}

int pci_write_config_byte(struct pci_dev *dev, int where, uint8_t val)
{
    uint32_t addr = (uint32_t) (1 << 31) | (dev->bus << 16) | (dev->dev << 11) |
        (dev->fn << 8) | (where & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    out_8((void *) (bridge.cfg_data) + (where & 3), val);
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    outb(val, PCI_CONFIG_DATA + (where & 3));
#endif
    return OK;
}

int pci_write_config_word(struct pci_dev *dev, int where, uint16_t val)
{
    uint32_t addr = (uint32_t) (1 << 31) | (dev->bus << 16) | (dev->dev << 11) |
        (dev->fn << 8) | (where & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    out_le16((void *) (bridge.cfg_data) + (where & 3), val);
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    outw(val, PCI_CONFIG_DATA + (where & 2));
#endif
    return OK;
}

int pci_write_config_dword(struct pci_dev *dev, int where, uint32_t val)
{
    uint32_t addr = (uint32_t) (1 << 31) | (dev->bus << 16) | (dev->dev << 11) |
        (dev->fn << 8) | (where & 0xfc);

#ifdef __PPC__
    out_be32((uint32_t *) bridge.cfg_addr, addr);
    out_le32((void *) (bridge.cfg_data) + (where & 3), val);
#else
    outl(PCI_CONFIG_ADDRESS, addr);
    *val = inl(PCI_CONFIG_DATA);
#endif
    return OK;
}



uint8_t  ioread8 (uint8_t  *addr)
{
#ifdef __PPC__
    return in_8(addr);
#endif
}

uint16_t ioread16(uint16_t *addr)
{
#ifdef __PPC__
    return in_le16(addr);
#endif
}

uint32_t ioread32(uint32_t *addr)
{
#ifdef __PPC__
    return in_le32(addr);
#endif
}

void iowrite8(uint8_t value, uint8_t *addr)
{
#ifdef __PPC__
    out_8(addr, value);
#endif
}

void iowrite16(uint16_t value, uint16_t *addr)
{
#ifdef __PPC__
    out_le16(addr, value);
#endif
}

void iowrite32(uint32_t value, uint32_t *addr)
{
#ifdef __PPC__
    out_le32(addr, value);
#endif
}


static void pci_fill_resource(struct pci_dev *dev, int res_idx)
{
    //TODO :
    // 1. Decode (I/O or memory) of a register is disabled via the command
    //       register before sizing a Base Address register.
    // 2. The original value in the Base Address register is restored before re-enabling
    //       decode in the command register of the device.
    // 3. "Note that the upper 16 bits of the result is ignored if the Base
    //       Address register is for I/O and bits 16-31 returned zero upon read."
    // 4. 64 bits bars
    //
    int reg;
    uint32_t val, size, mask;

    struct pci_resource * resource = &dev->resources[res_idx];
    memset(resource, 0, sizeof(*resource));


    if (res_idx !=PCI_RESOURCE_ROM) {
        reg = PCI_BASE_ADDRESS_0 + res_idx*4;
        mask =  0xffffffff;
    } else {
        reg = PCI_ROM_ADDRESS;
        mask = PCI_ROM_ADDRESS_MASK;
    }

    pci_read_config_dword(dev, reg, &val);
    pci_write_config_dword(dev, reg, mask);
    pci_read_config_dword(dev, reg, &size);
    pci_write_config_dword(dev, reg, val);

    if (res_idx == PCI_RESOURCE_ROM)  {
        resource->type = PCI_RESOURCE_TYPE_ROM;
        resource->addr = val & PCI_ROM_ADDRESS_MASK;
        size &= PCI_ROM_ADDRESS_MASK;

    } else if ((val&PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
        resource->type = PCI_RESOURCE_TYPE_BAR_IO;
        resource->addr = val & PCI_BASE_ADDRESS_IO_MASK;
        size &= PCI_BASE_ADDRESS_IO_MASK;

    } else {
        if ((val & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_32) {
            resource->mem_flags |= PCI_RESOURCE_MEM_MASK_32;
        }
        if (val & PCI_BASE_ADDRESS_MEM_PREFETCH) {
            resource->mem_flags |= PCI_RESOURCE_MEM_MASK_PREFETCH;
        }
        resource->type = PCI_RESOURCE_TYPE_BAR_MEM;
        resource->addr = val & PCI_BASE_ADDRESS_MEM_MASK;
        size &= PCI_BASE_ADDRESS_MEM_MASK;
    }

    resource->size = ~size + 1;
}

/*
 * see comment in pci.h
 */
void pci_get_dev_by_bdf(uint8_t bus, uint8_t dev, uint8_t fn, struct pci_dev *pci_dev)
{
    memset(pci_dev, 0, sizeof(*pci_dev));

    pci_dev->bus = bus;
    pci_dev->dev = dev;
    pci_dev->fn = fn;

    pci_read_config_word(pci_dev, PCI_VENDOR_ID, &pci_dev->vendor_id);
    //if (vendor_id == 0xFFFF) {
    //}
    pci_read_config_word(pci_dev, PCI_DEVICE_ID,    &pci_dev->device_id);
    pci_read_config_word(pci_dev, PCI_CLASS_DEVICE, &pci_dev->class_code);
    pci_read_config_byte(pci_dev, PCI_HEADER_TYPE,  &pci_dev->hdr_type);

    if (pci_dev->hdr_type != 0)
        return;

    for (int i = 0; i < PCI_NUM_RESOURCES; i++) {
        pci_fill_resource(pci_dev, i);
    }

}

void pci_enumerate()
{
    struct pci_dev pci_dev;
    for (unsigned int bus = 0; bus < PCI_BUS_MAX; bus++)
      for (unsigned int dev = 0; dev < PCI_DEV_MAX; dev++)
        for (unsigned int fn = 0; fn < PCI_FN_MAX; fn++)
        {
            pci_get_dev_by_bdf(bus, dev, fn, &pci_dev);

            if (pci_dev.vendor_id == 0xFFFF) {
                continue;
            }

            printf("%02x:%02x:%02x %s\n",
                    bus, dev, fn, get_pci_class_name(pci_dev.class_code));
            printf("\t PCI device %04x:%04x (header 0x%02x)\n",
                    pci_dev.vendor_id,
                    pci_dev.device_id,
                    pci_dev.hdr_type);

            if (pci_dev.hdr_type != 0) {
                continue;
            }

            for (int i = 0; i < PCI_NUM_RESOURCES; i++) {
                struct pci_resource *res = &pci_dev.resources[i];
                if (res->size == 0)
                    continue;

                if (res->type != PCI_RESOURCE_TYPE_ROM){
                    printf("\t BAR%d: ", i);
                    if (res->type == PCI_RESOURCE_TYPE_BAR_IO) {
                        printf("I/O ");
                    } else {
                        printf("%s bit %smemory ",
                            res->mem_flags & PCI_RESOURCE_MEM_MASK_32 ? "32": "<UNSUPPORTED> 64",
                            res->mem_flags & PCI_RESOURCE_MEM_MASK_PREFETCH ? "prefetchable ":"");
                    }
                }
                else {
                    printf("\t ROM: ");
                }
                printf("[size=0x%zx]\n", res->size);

            }
        }
}


void pci_list()
{
    struct pci_dev pci_dev;
    for (unsigned int bus = 0; bus < PCI_BUS_MAX; bus++)
      for (unsigned int dev = 0; dev < PCI_DEV_MAX; dev++)
        for (unsigned int fn = 0; fn < PCI_FN_MAX; fn++)
        {
            pci_get_dev_by_bdf(bus, dev, fn, &pci_dev);

            if (pci_dev.vendor_id == 0xFFFF) {
                continue;
            }

            printf("%02x:%02x:%02x %s\n",
                    bus, dev, fn, get_pci_class_name(pci_dev.class_code));
            printf("\t PCI device %04x:%04x (header 0x%02x)\n",
                    pci_dev.vendor_id,
                    pci_dev.device_id,
                    pci_dev.hdr_type);

            if (pci_dev.hdr_type != 0) {
                continue;
            }

            for (int i = 0; i < PCI_NUM_RESOURCES; i++) {
                struct pci_resource *res = &pci_dev.resources[i];
                if (res->size == 0)
                    continue;

                if (res->type != PCI_RESOURCE_TYPE_ROM){
                    printf("\t BAR%d: ", i);
                    if (res->type == PCI_RESOURCE_TYPE_BAR_IO) {
                        printf("I/O ");
                    } else {
                        printf("%s bit %smem ",
                            res->mem_flags & PCI_RESOURCE_MEM_MASK_32 ? "32": "<UNSUPPORTED> 64",
                            res->mem_flags & PCI_RESOURCE_MEM_MASK_PREFETCH ? "prefetch ":"");
                    }
                }
                else {
                    printf("\t ROM: ");
                }


                uint32_t bar0 = res->addr;
                if (bar0) {
                    printf("0x%lx ", bar0);
#ifdef __PPC__
                    if(res->type == PCI_RESOURCE_TYPE_BAR_IO) {
                        printf("(addr = 0x%lx) ", res->addr + bridge.iorange);
                    }
#endif
                }

                printf("[size=0x%zx]\n", res->size);

            }
        }
}

//dev will be usefull when bridge is not hardcoded
uintptr_t pci_convert_legacy_port(struct pci_dev *dev, uint16_t port)
{
    (void) dev;
#ifdef __PPC__
    return bridge.iorange + port;
#else
    return port;
#endif
}

#define PEX1_PEXOWBAR1 0xc28
#define PEX1_PEXOWAR1  0xc30
#define WAR_EN         0x80000000
#define WAR_RTT_IO     0x00080000
#define WAR_WTT_IO     0x00008000
#define WAR_OWS_8K     0xC

//addr should be 16MB aligned (this is the size of qemu vga mem arrea)
//and 3rd bit equal to 1 means prefetchable memory
#define VGA_ADDR 0xee000008
void vga_init(void);

void pci_init()
{
    printf("\nPCI\n");

#ifdef __PPC__
    pok_bsp_t pok_bsp;
    pok_bsp_get_info(&pok_bsp);
    bridge = pok_bsp.pci_bridge;

    //TODO add pci_bridge too?
    static uint32_t bar0_addr = 0x1001;
    const uint32_t BAR0_SIZE = 0x100;

    printf("bridge cfg_addr: %p cfg_data: %p\n",
            (void *)bridge.cfg_addr, (void *)bridge.cfg_data);

    out_be32((uint32_t *) (bridge.cfg_addr + PEX1_PEXOWBAR1), bridge.iorange>>12);
    out_be32((uint32_t *) (bridge.cfg_addr + PEX1_PEXOWAR1), WAR_EN|WAR_RTT_IO|WAR_WTT_IO|WAR_OWS_8K);

#endif

    printf("PCI enumeration:\n");
    pci_enumerate();

    printf("PCI initialization\n");
    for (unsigned int bus = 0; bus < PCI_BUS_MAX; bus++)
      for (unsigned int dev = 0; dev < PCI_DEV_MAX; dev++)
      {
            int fn = 0;
            // Currently we do not handle type 1 or 2 PCI configuration spaces
            if ((pci_read(bus, dev, fn, PCI_HEADER_TYPE) & 0xFF) != 0)
                continue;
            printf(">>>%02x:%02x:%02x \n", bus, dev, fn);
            struct pci_dev pci_dev;
            pci_dev.bus = bus;
            pci_dev.dev = dev;
            pci_dev.fn = fn;
            pci_dev.vendor_id  = (uint16_t) pci_read(bus, dev, fn, PCI_VENDOR_ID);
            pci_dev.device_id  = (uint16_t) pci_read(bus, dev, fn, PCI_DEVICE_ID);
            //pci_dev.classcode = (uint16_t) pci_read(bus, dev, fn, PCI_REG_PROGIFID);
            if (dev == 1) {
                pci_write_dword(&pci_dev, PCI_BASE_ADDRESS_0, VGA_ADDR);
                pci_write_word(&pci_dev, PCI_COMMAND, PCI_COMMAND_MEMORY);
                pci_write_dword(&pci_dev, PCI_ROM_ADDRESS, 0xedf00000|PCI_ROM_ADDRESS_ENABLE);
                {
                    pci_write_dword(&pci_dev, PCI_BASE_ADDRESS_2, 0xede00000);
                    pci_write_word(&pci_dev, PCI_COMMAND, PCI_COMMAND_MEMORY);
                }

                vga_init();
                continue;
            }

            for (int i = 0; i < pci_driver_table_used_cnt; i++) {
                struct pci_driver *pci_driver = &pci_driver_table[i];
                if (pci_match_device(pci_driver->id_table, &pci_dev)) {
                    printf("MATCH %s\n", pci_driver->name);
#ifdef __PPC__
                    pci_write_dword(&pci_dev, PCI_BASE_ADDRESS_0, bar0_addr);
                    pci_write_word(&pci_dev, PCI_COMMAND, PCI_COMMAND_IO);
                    pci_dev.bar[0] = bridge.iorange + bar0_addr;
                    pci_dev.ioaddr = pci_dev.bar[0] & BAR_IOADDR_MASK;
                    bar0_addr += BAR0_SIZE;
#else
                    pci_dev.bar[0] = pci_read(bus, dev, fn, PCI_BASE_ADDRESS_0) & BAR_IOADDR_MASK; //TODO!!!!
                    pci_dev.ioaddr = pci_dev.bar[0] & BAR_IOADDR_MASK;
#endif
                    pci_driver->probe(&pci_dev);
                }
            }
        }
    printf("PCI init result:\n");
    pci_list();
    printf("\n");
}
