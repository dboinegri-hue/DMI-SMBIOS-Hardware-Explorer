/* =============================================================================
 *  display_system.c -- Pretty-printers for system / BIOS / motherboard / CPU
 *  -----------------------------------------------------------------------------
 *  Author : Leyla
 *
 *  Handles SMBIOS structure types:
 *      Type 0   BIOS Information
 *      Type 1   System Information
 *      Type 2   Baseboard (Motherboard) Information   <-- assignment focus
 *      Type 4   Processor Information
 *
 *  Field offsets come straight from the SMBIOS reference spec (DSP0134).
 *  Every field access is guarded by `length >= required` so we never read
 *  past the formatted area on older / partial structures.
 * =============================================================================
 */
#include "display.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Tiny safe accessors -- returns 0 if the structure is too short. */
static uint8_t  rb(const SmbiosHeader *h, size_t off) {
    return (off < h->length) ? ((const uint8_t *)h)[off] : 0;
}
static uint16_t rw(const SmbiosHeader *h, size_t off) {
    if (off + 2 > h->length) return 0;
    const uint8_t *p = (const uint8_t *)h + off;
    return (uint16_t)(p[0] | (p[1] << 8));
}

/* ---- Type 1: System Information --------------------------------------- */

void display_system_info(const SmbiosTable *tbl)
{
    const SmbiosHeader *h = smbios_find(tbl, 1);
    util_section("System Information (SMBIOS Type 1)");
    if (!h) { puts("  (no Type 1 record found)"); return; }

    util_kv("Manufacturer",  smbios_string(h, rb(h, 0x04)));
    util_kv("Product Name",  smbios_string(h, rb(h, 0x05)));
    util_kv("Version",       smbios_string(h, rb(h, 0x06)));
    util_kv("Serial Number", smbios_string(h, rb(h, 0x07)));

    /* UUID at offset 0x08, 16 bytes */
    if (h->length >= 0x18) {
        const uint8_t *u = (const uint8_t *)h + 0x08;
        char uuid[64];
        snprintf(uuid, sizeof(uuid),
            "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            u[3],u[2],u[1],u[0],     /* first 3 fields are little-endian */
            u[5],u[4],
            u[7],u[6],
            u[8],u[9],
            u[10],u[11],u[12],u[13],u[14],u[15]);
        util_kv("UUID", uuid);
    }

    if (h->length >= 0x1A) {
        util_kv("SKU Number", smbios_string(h, rb(h, 0x19)));
        util_kv("Family",     smbios_string(h, rb(h, 0x1A)));
    }
}

/* ---- Type 0: BIOS Information ----------------------------------------- */

void display_bios_info(const SmbiosTable *tbl)
{
    const SmbiosHeader *h = smbios_find(tbl, 0);
    util_section("BIOS Information (SMBIOS Type 0)");
    if (!h) { puts("  (no Type 0 record found)"); return; }

    util_kv("Vendor",        smbios_string(h, rb(h, 0x04)));
    util_kv("BIOS Version",  smbios_string(h, rb(h, 0x05)));
    util_kv("Release Date",  smbios_string(h, rb(h, 0x08)));

    /* ROM size: byte at 0x09. Special value 0xFF means use extended field. */
    uint8_t romByte = rb(h, 0x09);
    if (romByte != 0xFF) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u KB", (romByte + 1) * 64u);
        util_kv("ROM Size", buf);
    } else if (h->length >= 0x1A) {
        uint16_t ext = rw(h, 0x18);
        uint16_t unitCode = (ext >> 14) & 0x3;
        uint16_t value    = ext & 0x3FFF;
        const char *unit = (unitCode == 0) ? "MB" : "GB";
        char buf[32];
        snprintf(buf, sizeof(buf), "%u %s", value, unit);
        util_kv("ROM Size", buf);
    }

    if (h->length >= 0x16) {
        char ver[32];
        snprintf(ver, sizeof(ver), "%u.%u",
                 rb(h, 0x14), rb(h, 0x15));
        util_kv("System BIOS Release", ver);
    }
    if (h->length >= 0x18) {
        char ver[32];
        snprintf(ver, sizeof(ver), "%u.%u",
                 rb(h, 0x16), rb(h, 0x17));
        util_kv("Embedded Ctrl Release", ver);
    }
}

/* ---- Type 2: Baseboard / Motherboard ---------------------------------- */

void display_baseboard_info(const SmbiosTable *tbl)
{
    util_section("Motherboard / Baseboard (SMBIOS Type 2)");

    /* There can be more than one Type 2 record (some servers expose riser
     * boards too) so we walk and number them. */
    const SmbiosHeader *h = NULL;
    int idx = 0;
    while ((h = smbios_next(tbl, h)) != NULL) {
        if (h->type != 2) continue;
        ++idx;
        if (idx > 1) printf("\n  -- Board #%d --\n", idx);

        util_kv("Manufacturer",      smbios_string(h, rb(h, 0x04)));
        util_kv("Product",           smbios_string(h, rb(h, 0x05)));
        util_kv("Version",           smbios_string(h, rb(h, 0x06)));
        util_kv("Serial Number",     smbios_string(h, rb(h, 0x07)));
        util_kv("Asset Tag",         smbios_string(h, rb(h, 0x08)));
        util_kv("Location in Chassis", smbios_string(h, rb(h, 0x0A)));

        if (h->length >= 0x0E) {
            util_kv("Board Type", util_baseboard_type(rb(h, 0x0D)));
        }

        /* Feature flags at offset 0x09 */
        if (h->length >= 0x0A) {
            uint8_t f = rb(h, 0x09);
            char flags[256] = "";
            if (f & 0x01) strcat(flags, "Hosting Board; ");
            if (f & 0x02) strcat(flags, "Requires daughter; ");
            if (f & 0x04) strcat(flags, "Removable; ");
            if (f & 0x08) strcat(flags, "Replaceable; ");
            if (f & 0x10) strcat(flags, "Hot Swappable; ");
            if (flags[0] == '\0') strcpy(flags, "(none)");
            util_kv("Feature Flags", flags);
        }
    }
    if (idx == 0) puts("  (no Type 2 record found)");
}

/* ---- Type 4: Processor ------------------------------------------------ */

void display_processor_info(const SmbiosTable *tbl)
{
    util_section("Processor Information (SMBIOS Type 4)");
    const SmbiosHeader *h = NULL;
    int idx = 0;
    while ((h = smbios_next(tbl, h)) != NULL) {
        if (h->type != 4) continue;
        ++idx;
        if (idx > 1) printf("\n  -- CPU #%d --\n", idx);

        util_kv("Socket",        smbios_string(h, rb(h, 0x04)));
        util_kv("Type",          util_processor_type(rb(h, 0x05)));

        /* Family: byte at 0x06; if 0xFE, use Family 2 (word at 0x28) */
        uint16_t fam = rb(h, 0x06);
        if (fam == 0xFE && h->length >= 0x2A) fam = rw(h, 0x28);
        util_kv("Family",        util_processor_family(fam));

        util_kv("Manufacturer",  smbios_string(h, rb(h, 0x07)));
        util_kv("Version",       smbios_string(h, rb(h, 0x10)));

        if (h->length >= 0x16) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u MHz", rw(h, 0x14));
            util_kv("Max Speed", buf);
            snprintf(buf, sizeof(buf), "%u MHz", rw(h, 0x16));
            util_kv("Current Speed", buf);
        }
        if (h->length >= 0x14) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u MHz", rw(h, 0x12));
            util_kv("External Clock", buf);
        }

        /* Cores / threads (with v3 extension if present) */
        if (h->length >= 0x26) {
            uint16_t cores  = rb(h, 0x23);
            uint16_t enabled= rb(h, 0x24);
            uint16_t thrds  = rb(h, 0x25);
            if (h->length >= 0x30) {
                if (cores == 0xFF)   cores   = rw(h, 0x2A);
                if (enabled == 0xFF) enabled = rw(h, 0x2C);
                if (thrds == 0xFF)   thrds   = rw(h, 0x2E);
            }
            util_kv_u("Core Count",     cores,   "");
            util_kv_u("Cores Enabled",  enabled, "");
            util_kv_u("Thread Count",   thrds,   "");
        }
        if (h->length >= 0x23) {
            util_kv("Serial Number", smbios_string(h, rb(h, 0x20)));
            util_kv("Part Number",   smbios_string(h, rb(h, 0x22)));
        }
    }
    if (idx == 0) puts("  (no Type 4 record found)");
}

/* ---- Summary of the loaded table -------------------------------------- */

void display_table_summary(const SmbiosTable *tbl)
{
    util_section("SMBIOS Table Summary");
    char ver[32];
    snprintf(ver, sizeof(ver), "%u.%u (DMI rev %u)",
             tbl->smbiosMajorVersion, tbl->smbiosMinorVersion,
             tbl->dmiRevision);
    util_kv("SMBIOS Version", ver);
    util_kv_u("Table Length", tbl->length, "bytes");

    /* Count structures by walking. */
    unsigned total = 0;
    const SmbiosHeader *h = NULL;
    while ((h = smbios_next(tbl, h)) != NULL) ++total;
    util_kv_u("Structure Count", total, "");
}

void display_all(const SmbiosTable *tbl)
{
    display_table_summary(tbl);
    display_system_info(tbl);
    display_bios_info(tbl);
    display_baseboard_info(tbl);
    display_processor_info(tbl);
    display_memory_array(tbl);
    display_memory_devices(tbl);
}
