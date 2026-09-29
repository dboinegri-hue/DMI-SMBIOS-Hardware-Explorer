/* =============================================================================
 *  smbios.c -- SMBIOS table loader and structure walker (Linux)
 *  -----------------------------------------------------------------------------
 *  Author : [Partner Name]
 *
 *  We read the firmware-supplied SMBIOS table from sysfs, the canonical
 *  Linux-kernel-exposed location:
 *
 *      /sys/firmware/dmi/tables/DMI                  -- raw structure stream
 *      /sys/firmware/dmi/tables/smbios_entry_point   -- header with version
 *
 *  Each SMBIOS structure is laid out as:
 *
 *      [  formatted area (header.length bytes)  ][ string1\0string2\0...\0\0 ]
 *
 *  Walking to the next structure means: skip past the formatted area, then
 *  scan forward until we see two consecutive null bytes, which terminate
 *  the string table. The first byte after that double-null is the next
 *  structure's header (or end-of-buffer).
 *
 *  Note: on most Ubuntu / Debian systems the DMI file is readable by
 *  any user. On a few minimal installs the file is mode 0400 and the
 *  program must be invoked with sudo. The Linux backend reports a
 *  permission error in that case so the caller can suggest sudo.
 * =============================================================================
 */
#include "smbios.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int smbios_load(SmbiosTable *out, unsigned long *errOut)
{
    if (!out) { if (errOut) *errOut = 0xFFFFFFFFUL; return 1; }
    memset(out, 0, sizeof(*out));

    /* ---- Step 1: read the entry-point header for the SMBIOS version ---- */
    FILE *epf = fopen("/sys/firmware/dmi/tables/smbios_entry_point", "rb");
    if (epf) {
        unsigned char ep[64];
        size_t epLen = fread(ep, 1, sizeof(ep), epf);
        fclose(epf);

        /* SMBIOS 3.x uses a 64-bit anchor "_SM3_"; 2.x uses "_SM_".
         * Field offsets per DMTF DSP0134. */
        if (epLen >= 10 && memcmp(ep, "_SM3_", 5) == 0) {
            out->smbiosMajorVersion = ep[7];
            out->smbiosMinorVersion = ep[8];
            out->dmiRevision        = ep[9];
        } else if (epLen >= 8 && memcmp(ep, "_SM_", 4) == 0) {
            out->smbiosMajorVersion = ep[6];
            out->smbiosMinorVersion = ep[7];
            out->dmiRevision        = 0;
        }
        /* Either way, proceed -- the table itself is what we really want. */
    }
    /* Missing entry-point file is non-fatal; we'll just report v0.0. */

    /* ---- Step 2: read the raw DMI table in full ----------------------- */
    FILE *f = fopen("/sys/firmware/dmi/tables/DMI", "rb");
    if (!f) {
        if (errOut) *errOut = (unsigned long)errno;
        return 2;  /* file missing or permission denied */
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 4; }
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return 4; }
    rewind(f);

    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); if (errOut) *errOut = 0; return 3; }

    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) { free(buf); return 4; }

    /* The sysfs file IS the table -- no wrapper struct to peel. */
    out->length     = (uint32_t)sz;
    out->tableData  = buf;
    out->rawBuffer  = buf;
    out->rawSize    = (uint32_t)sz;

    if (errOut) *errOut = 0;
    return 0;
}

void smbios_free(SmbiosTable *tbl)
{
    if (!tbl) return;
    if (tbl->rawBuffer) free(tbl->rawBuffer);
    memset(tbl, 0, sizeof(*tbl));
}

/* ------------------------------------------------------------------------- */
/* Walking helpers                                                            */
/* ------------------------------------------------------------------------- */

const uint8_t *smbios_struct_end(const SmbiosHeader *hdr)
{
    if (!hdr) return NULL;
    const uint8_t *p = (const uint8_t *)hdr + hdr->length;

    /* Walk past the string area: each string is null-terminated; the table
     * ends at a double-null. Even if there are zero strings, SMBIOS guarantees
     * two null bytes after the formatted area. */
    while (p[0] != 0 || p[1] != 0) {
        ++p;
    }
    return p + 2; /* skip the double null */
}

const SmbiosHeader *smbios_next(const SmbiosTable *tbl,
                                const SmbiosHeader *current)
{
    if (!tbl || !tbl->tableData) return NULL;

    const uint8_t *next;
    if (current == NULL) {
        next = tbl->tableData;
    } else {
        next = smbios_struct_end(current);
    }

    const uint8_t *tableEnd = tbl->tableData + tbl->length;
    if (next >= tableEnd) return NULL;

    const SmbiosHeader *hdr = (const SmbiosHeader *)next;

    /* Type 127 == End-of-table. Treat as iterator stop. */
    if (hdr->type == 127) return NULL;
    if (hdr->length < sizeof(SmbiosHeader)) return NULL; /* corrupt */

    return hdr;
}

const SmbiosHeader *smbios_find(const SmbiosTable *tbl, uint8_t type)
{
    const SmbiosHeader *h = NULL;
    while ((h = smbios_next(tbl, h)) != NULL) {
        if (h->type == type) return h;
    }
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* String lookup                                                              */
/* ------------------------------------------------------------------------- */

const char *smbios_string(const SmbiosHeader *hdr, uint8_t index)
{
    if (!hdr) return "";
    if (index == 0) return "";  /* SMBIOS: 0 means "no string" */

    const char *p = (const char *)hdr + hdr->length;
    uint8_t i = 1;
    while (*p != '\0') {
        if (i == index) {
            /* Trim purely-whitespace results to "<not specified>" so the
             * UI doesn't display rows of empty fields for unfilled BIOSes. */
            const char *q = p;
            while (*q == ' ' || *q == '\t') ++q;
            if (*q == '\0') return "<not specified>";
            return p;
        }
        p += strlen(p) + 1;
        ++i;
    }
    return "<bad index>";
}
