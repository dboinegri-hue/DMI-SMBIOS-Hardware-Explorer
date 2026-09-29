/* =============================================================================
 *  display_memory.c -- Pretty-printers for memory arrays and RAM slots
 *  -----------------------------------------------------------------------------
 *  Author : [Partner Name]
 *
 *  This module is the heart of the assignment: enumerating every physical
 *  memory slot (DIMM/SODIMM) on the board, including empty ones, and showing
 *  size, speed, type, manufacturer, etc.
 *
 *  Handles SMBIOS structure types:
 *      Type 16  Physical Memory Array (one per memory controller / channel set)
 *      Type 17  Memory Device         (one per slot, populated or not)
 * =============================================================================
 */
#include "display.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static uint8_t  rb(const SmbiosHeader *h, size_t off) {
    return (off < h->length) ? ((const uint8_t *)h)[off] : 0;
}
static uint16_t rw(const SmbiosHeader *h, size_t off) {
    if (off + 2 > h->length) return 0;
    const uint8_t *p = (const uint8_t *)h + off;
    return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t rd(const SmbiosHeader *h, size_t off) {
    if (off + 4 > h->length) return 0;
    const uint8_t *p = (const uint8_t *)h + off;
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* ---- Type 16: Physical Memory Array ----------------------------------- */

void display_memory_array(const SmbiosTable *tbl)
{
    util_section("Memory Array (SMBIOS Type 16)");
    const SmbiosHeader *h = NULL;
    int idx = 0;
    while ((h = smbios_next(tbl, h)) != NULL) {
        if (h->type != 16) continue;
        ++idx;
        if (idx > 1) printf("\n  -- Memory Array #%d --\n", idx);

        util_kv("Location",         util_memory_array_location(rb(h, 0x04)));
        util_kv("Use",              util_memory_array_use(rb(h, 0x05)));
        util_kv("Error Correction", util_memory_ec_type(rb(h, 0x06)));

        /* Maximum Capacity is a DWORD at 0x07 in KB; if 0x80000000, fall back
         * to the v2.7+ Extended Maximum Capacity QWORD at offset 0x0F. */
        uint32_t maxKB = rd(h, 0x07);
        char sizeBuf[64];
        if (maxKB == 0x80000000UL && h->length >= 0x17) {
            uint64_t lo = rd(h, 0x0F);
            uint64_t hi = rd(h, 0x13);
            uint64_t bytes = (hi << 32) | lo;
            uint64_t mib = bytes / (1024ULL * 1024ULL);
            util_format_size_mib((uint32_t)mib, sizeBuf, sizeof(sizeBuf));
        } else {
            uint32_t mib = maxKB / 1024u;
            util_format_size_mib(mib, sizeBuf, sizeof(sizeBuf));
        }
        util_kv("Maximum Capacity", sizeBuf);

        if (h->length >= 0x0F) {
            uint16_t numDevices = rw(h, 0x0D);
            util_kv_u("Number of Slots", numDevices, "");
        }
    }
    if (idx == 0) puts("  (no Type 16 record found)");
}

/* ---- Type 17: Memory Devices (RAM slots) ------------------------------ */

static void describe_type_detail(uint16_t td, char *out, size_t outSize)
{
    out[0] = '\0';
    if (td == 0 || td == 1) { snprintf(out, outSize, "Unknown"); return; }
    if (td & 0x0002) strncat(out, "Other; ",        outSize - strlen(out) - 1);
    if (td & 0x0004) strncat(out, "Unknown; ",      outSize - strlen(out) - 1);
    if (td & 0x0008) strncat(out, "Fast-paged; ",   outSize - strlen(out) - 1);
    if (td & 0x0010) strncat(out, "Static col; ",   outSize - strlen(out) - 1);
    if (td & 0x0020) strncat(out, "Pseudo-static; ",outSize - strlen(out) - 1);
    if (td & 0x0040) strncat(out, "RAMBUS; ",       outSize - strlen(out) - 1);
    if (td & 0x0080) strncat(out, "Synchronous; ",  outSize - strlen(out) - 1);
    if (td & 0x0100) strncat(out, "CMOS; ",         outSize - strlen(out) - 1);
    if (td & 0x0200) strncat(out, "EDO; ",          outSize - strlen(out) - 1);
    if (td & 0x0400) strncat(out, "Window DRAM; ",  outSize - strlen(out) - 1);
    if (td & 0x0800) strncat(out, "Cache DRAM; ",   outSize - strlen(out) - 1);
    if (td & 0x1000) strncat(out, "Non-volatile; ", outSize - strlen(out) - 1);
    if (td & 0x2000) strncat(out, "Registered; ",   outSize - strlen(out) - 1);
    if (td & 0x4000) strncat(out, "Unbuffered; ",   outSize - strlen(out) - 1);
    if (td & 0x8000) strncat(out, "LRDIMM; ",       outSize - strlen(out) - 1);
    if (out[0] == '\0') snprintf(out, outSize, "(none)");
}

void display_memory_devices(const SmbiosTable *tbl)
{
    util_section("Memory Devices / RAM Slots (SMBIOS Type 17)");

    int slotIndex = 0;
    int populated = 0;
    uint64_t totalMiB = 0;
    const SmbiosHeader *h = NULL;

    while ((h = smbios_next(tbl, h)) != NULL) {
        if (h->type != 17) continue;
        ++slotIndex;
        printf("\n  ---- Slot %d ----\n", slotIndex);

        util_kv("Locator",      smbios_string(h, rb(h, 0x10)));
        util_kv("Bank Locator", smbios_string(h, rb(h, 0x11)));

        /* Size (WORD at 0x0C). 0 = empty, 0x7FFF = use Extended Size DWORD at 0x1C */
        uint16_t sizeField = rw(h, 0x0C);
        uint32_t sizeMiB = 0;
        const char *sizeNote = "";

        if (sizeField == 0) {
            util_kv("Size", "(slot empty -- no module installed)");
        } else {
            if (sizeField == 0x7FFF) {
                if (h->length >= 0x20) {
                    sizeMiB = rd(h, 0x1C) & 0x7FFFFFFFUL;
                }
            } else {
                /* Bit 15: 0 = MB, 1 = KB */
                if (sizeField & 0x8000) {
                    /* size in KB */
                    sizeMiB = (sizeField & 0x7FFF) / 1024u;
                    sizeNote = " (sub-MB module)";
                } else {
                    sizeMiB = sizeField & 0x7FFF;
                }
            }
            char buf[64];
            util_format_size_mib(sizeMiB, buf, sizeof(buf));
            char line[96];
            snprintf(line, sizeof(line), "%s%s", buf, sizeNote);
            util_kv("Size", line);
            ++populated;
            totalMiB += sizeMiB;
        }

        util_kv("Form Factor",  util_memory_form_factor(rb(h, 0x0E)));
        util_kv("Memory Type",  util_memory_type(rb(h, 0x12)));

        if (h->length >= 0x15) {
            char td[256];
            describe_type_detail(rw(h, 0x13), td, sizeof(td));
            util_kv("Type Detail", td);
        }

        /* Total / Data width */
        if (h->length >= 0x0C) {
            uint16_t total = rw(h, 0x08);
            uint16_t data  = rw(h, 0x0A);
            char buf[64];
            if (total == 0xFFFF) snprintf(buf, sizeof(buf), "Unknown");
            else                 snprintf(buf, sizeof(buf), "%u bits", total);
            util_kv("Total Width", buf);
            if (data == 0xFFFF)  snprintf(buf, sizeof(buf), "Unknown");
            else                 snprintf(buf, sizeof(buf), "%u bits", data);
            util_kv("Data Width",  buf);
        }

        /* Speed */
        if (sizeField != 0 && h->length >= 0x17) {
            uint16_t spd = rw(h, 0x15);
            char buf[64];
            if (spd == 0)          snprintf(buf, sizeof(buf), "Unknown");
            else                   snprintf(buf, sizeof(buf), "%u MT/s", spd);
            util_kv("Rated Speed", buf);

            if (h->length >= 0x22) {
                uint16_t cspd = rw(h, 0x20);
                if (cspd == 0)     snprintf(buf, sizeof(buf), "Unknown");
                else               snprintf(buf, sizeof(buf), "%u MT/s", cspd);
                util_kv("Configured Speed", buf);
            }
        }

        /* Manufacturer / part / serial */
        if (sizeField != 0 && h->length >= 0x1B) {
            util_kv("Manufacturer", smbios_string(h, rb(h, 0x17)));
            util_kv("Serial Number",smbios_string(h, rb(h, 0x18)));
            util_kv("Asset Tag",    smbios_string(h, rb(h, 0x19)));
            util_kv("Part Number",  smbios_string(h, rb(h, 0x1A)));
        }

        /* Rank (low nibble of Attributes byte) */
        if (sizeField != 0 && h->length >= 0x1C) {
            uint8_t attr = rb(h, 0x1B);
            uint8_t rank = attr & 0x0F;
            if (rank != 0) util_kv_u("Rank", rank, "");
        }
    }

    /* Summary */
    if (slotIndex == 0) {
        puts("  (no Type 17 records found)");
    } else {
        char total[64];
        util_format_size_mib((uint32_t)totalMiB, total, sizeof(total));
        printf("\n");
        util_hr();
        printf("  Total slots: %d   |   Populated: %d   |   Installed RAM: %s\n",
               slotIndex, populated, total);
        util_hr();
    }
}
