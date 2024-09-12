// vga.c - A VGA device
//

#include "vga.h"
#include "halt.h"
#include "console.h"

// This file is based on Sanjeevi Sangotuvel's code for MP1.
//

// From QEMU hw/pci.h and hw/display/bochs-vbe.h

#define PCI_CLASS_DISPLAY_VGA            0x0300
#define PCI_CLASS_DISPLAY_OTHER          0x0380
#define PCI_VENDOR_ID_QEMU               0x1234
#define PCI_DEVICE_ID_QEMU_VGA           0x1111

#define PCI_VGA_BOCHS_OFFSET  0x500

/* bochs vesa bios extension interface */
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

struct pci_config {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t rev_id;
    uint8_t prgif_id;

    union {
        struct {
            uint8_t sub_class;
            uint8_t base_class;
        };

        uint16_t full_class;
    };

    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6];
};

void vga_attach(uint64_t cfgaddr, uint16_t * fbuf) {
    volatile struct pci_config * cfg = (void*)cfgaddr;
    volatile uint16_t * vbe;
    void * ctlbase;
    uint32_t ctlsize;
    uint32_t fbsize;
    uint32_t bar;

    // Check for the expected emulated video card

    if (cfg->vendor_id != PCI_VENDOR_ID_QEMU ||
        cfg->device_id != PCI_DEVICE_ID_QEMU_VGA ||
        cfg->full_class != PCI_CLASS_DISPLAY_OTHER)
    {
        panic("QEMU Bochs VGA device not found");
    }

    // VRAM  regions must be in low memory

    assert (((uintptr_t)fbuf >> 32) == 0);

    // Configure adapter VRAM at /fbuf/ and control registers immediately
    // following that. The PCI configuration protocol entails, for each BAR:
    //   1. Writing all ones to the BAR.
    //   2. Reading back the value, where the bits 3:0 are flags, and the
    //      bits 31:4 are zero or one to indicate requested allocation size.
    //      if the least significant non-zero bit is k, then the requested
    //      allocation is 2^k.
    //   3. Allocating/reserving the address range for the device.
    //   4. Writing the base address of the MMIO region aloocated in step 3 to
    //      the BAR.
    // 
    // For the Bochs VGA device, BAR0 is the VRAM region, BAR1 is not used,
    // and BAR2 is the control register region. The control registers inlcude
    // the VBE control registers used to configure the device. See:
    // 
    //   http://wiki.osdev.org/Bochs_VBE_Extensions
    //   https://github.com/qemu/vgabios/blob/master/vbe_display_api.txt
    //


    cfg->bar[0] = -1;
    bar = cfg->bar[0];
    fbsize = (~bar | 0xF) + 1;

    if ((uintptr_t)fbuf & (fbsize - 1))
        panic("Misaligned fbuf");

    cfg->bar[0] = (uint32_t)(uintptr_t)fbuf;

    cfg->bar[2] = -1;
    bar = cfg->bar[2];
    ctlsize = (~bar | 0xF) + 1;
    ctlbase = fbuf + fbsize;

    assert (((uintptr_t)ctlbase & (ctlsize - 1)) == 0);
    cfg->bar[2] = (uint32_t)(uintptr_t)ctlbase;

    // Give device MMIO access

    cfg->command |= 2; // set bit 1

    // Configure display via VBE interface

    vbe = (void*)ctlbase + PCI_VGA_BOCHS_OFFSET;

    if (vbe[VBE_DISPI_INDEX_ID] < 0xB0C4)
        panic("Unexpected Bochs VBE ID");
    
    vbe[VBE_DISPI_INDEX_ENABLE] = VBE_DISPI_DISABLED;
    vbe[VBE_DISPI_INDEX_XRES] = VGA_WIDTH;
    vbe[VBE_DISPI_INDEX_YRES] = VGA_HEIGHT;
    vbe[VBE_DISPI_INDEX_BPP] = 16;
    vbe[VBE_DISPI_INDEX_ENABLE] = VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED;

    kprintf("Configured Bochs device for %dx%d\n", VGA_WIDTH, VGA_HEIGHT);
}
