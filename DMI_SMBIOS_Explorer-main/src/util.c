/* =============================================================================
 *  util.c -- Shared helpers (printing + SMBIOS enum decoding)
 *  -----------------------------------------------------------------------------
 *  Author : [Partner Name]
 * =============================================================================
 */
#include "util.h"
#include <stdio.h>
#include <string.h>

/* ----- Output helpers --------------------------------------------------- */

void util_hr(void)
{
    puts("------------------------------------------------------------");
}

void util_section(const char *title)
{
    printf("\n");
    util_hr();
    printf("  %s\n", title);
    util_hr();
}

void util_kv(const char *key, const char *val)
{
    if (val == NULL || *val == '\0') val = "<not specified>";
    printf("  %-26s : %s\n", key, val);
}

void util_kv_u(const char *key, unsigned val, const char *unit)
{
    if (unit == NULL) unit = "";
    printf("  %-26s : %u %s\n", key, val, unit);
}

char *util_format_size_mib(uint32_t mib, char *dst, size_t dstSize)
{
    if (mib == 0) {
        snprintf(dst, dstSize, "0");
        return dst;
    }
    if (mib >= 1024 && (mib % 1024) == 0) {
        snprintf(dst, dstSize, "%u GB", mib / 1024);
    } else if (mib >= 1024) {
        snprintf(dst, dstSize, "%.2f GB", (double)mib / 1024.0);
    } else {
        snprintf(dst, dstSize, "%u MB", mib);
    }
    return dst;
}

/* ----- SMBIOS enum lookups (DSP0134 v3.x) ------------------------------- */

const char *util_baseboard_type(uint8_t code)
{
    switch (code) {
        case 0x01: return "Unknown";
        case 0x02: return "Other";
        case 0x03: return "Server Blade";
        case 0x04: return "Connectivity Switch";
        case 0x05: return "System Management Module";
        case 0x06: return "Processor Module";
        case 0x07: return "I/O Module";
        case 0x08: return "Memory Module";
        case 0x09: return "Daughter Board";
        case 0x0A: return "Motherboard";
        case 0x0B: return "Processor/Memory Module";
        case 0x0C: return "Processor/IO Module";
        case 0x0D: return "Interconnect Board";
        default:   return "Unknown";
    }
}

const char *util_processor_type(uint8_t code)
{
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0x03: return "Central Processor";
        case 0x04: return "Math Processor";
        case 0x05: return "DSP Processor";
        case 0x06: return "Video Processor";
        default:   return "Unknown";
    }
}

const char *util_processor_family(uint16_t code)
{
    /* Tiny subset -- the full spec table is hundreds of entries.
     * We cover the values likely to show up on student/developer machines. */
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0xB1: return "Pentium III";
        case 0xB3: return "Intel Xeon";
        case 0xC6: return "AMD Athlon";
        case 0xCD: return "Intel Core 2";
        case 0xCE: return "AMD K8";
        case 0xD2: return "Intel Core 2 Solo";
        case 0xD6: return "AMD Phenom";
        case 0xD7: return "AMD Turion";
        case 0xDA: return "AMD V";
        case 0xDD: return "AMD Sempron";
        case 0xDE: return "AMD A-Series";
        case 0xE4: return "Intel Core i7";
        case 0xE5: return "Dual-Core Intel Celeron";
        case 0xE6: return "Intel Core 2 Duo";
        case 0xE9: return "Intel Core 2 Quad";
        case 0xEA: return "Intel Core i7";
        case 0xEB: return "Intel Core i5";
        case 0xEC: return "Intel Core i3";
        case 0xED: return "Intel Core i9";
        case 0xFE: return "(see Processor Family 2)";
        default:   return "Unknown / unmapped";
    }
}

const char *util_memory_form_factor(uint8_t code)
{
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0x03: return "SIMM";
        case 0x04: return "SIP";
        case 0x05: return "Chip";
        case 0x06: return "DIP";
        case 0x07: return "ZIP";
        case 0x08: return "Proprietary Card";
        case 0x09: return "DIMM";
        case 0x0A: return "TSOP";
        case 0x0B: return "Row of chips";
        case 0x0C: return "RIMM";
        case 0x0D: return "SODIMM";
        case 0x0E: return "SRIMM";
        case 0x0F: return "FB-DIMM";
        case 0x10: return "Die";
        default:   return "Unknown";
    }
}

const char *util_memory_type(uint8_t code)
{
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0x03: return "DRAM";
        case 0x04: return "EDRAM";
        case 0x05: return "VRAM";
        case 0x06: return "SRAM";
        case 0x07: return "RAM";
        case 0x08: return "ROM";
        case 0x09: return "FLASH";
        case 0x0A: return "EEPROM";
        case 0x0B: return "FEPROM";
        case 0x0C: return "EPROM";
        case 0x0D: return "CDRAM";
        case 0x0E: return "3DRAM";
        case 0x0F: return "SDRAM";
        case 0x10: return "SGRAM";
        case 0x11: return "RDRAM";
        case 0x12: return "DDR";
        case 0x13: return "DDR2";
        case 0x14: return "DDR2 FB-DIMM";
        case 0x18: return "DDR3";
        case 0x19: return "FBD2";
        case 0x1A: return "DDR4";
        case 0x1B: return "LPDDR";
        case 0x1C: return "LPDDR2";
        case 0x1D: return "LPDDR3";
        case 0x1E: return "LPDDR4";
        case 0x1F: return "Logical non-volatile device";
        case 0x20: return "HBM";
        case 0x21: return "HBM2";
        case 0x22: return "DDR5";
        case 0x23: return "LPDDR5";
        case 0x24: return "HBM3";
        default:   return "Unknown";
    }
}

const char *util_memory_array_location(uint8_t code)
{
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0x03: return "System Board / Motherboard";
        case 0x04: return "ISA Add-on Card";
        case 0x05: return "EISA Add-on Card";
        case 0x06: return "PCI Add-on Card";
        case 0x07: return "MCA Add-on Card";
        case 0x08: return "PCMCIA Add-on Card";
        case 0x09: return "Proprietary Add-on Card";
        case 0x0A: return "NuBus";
        default:   return "Unknown";
    }
}

const char *util_memory_array_use(uint8_t code)
{
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0x03: return "System Memory";
        case 0x04: return "Video Memory";
        case 0x05: return "Flash Memory";
        case 0x06: return "Non-volatile RAM";
        case 0x07: return "Cache Memory";
        default:   return "Unknown";
    }
}

const char *util_memory_ec_type(uint8_t code)
{
    switch (code) {
        case 0x01: return "Other";
        case 0x02: return "Unknown";
        case 0x03: return "None";
        case 0x04: return "Parity";
        case 0x05: return "Single-bit ECC";
        case 0x06: return "Multi-bit ECC";
        case 0x07: return "CRC";
        default:   return "Unknown";
    }
}
