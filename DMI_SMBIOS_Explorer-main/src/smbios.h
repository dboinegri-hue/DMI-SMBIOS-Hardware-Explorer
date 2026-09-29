/* =============================================================================
 *  smbios.h -- SMBIOS (DMI) table loader and walker (Linux)
 *  -----------------------------------------------------------------------------
 *  Project : DMI / SMBIOS Hardware Explorer
 *  Course  : Operating Systems
 *  Author  : [Partner Name]   (firmware-table acquisition + structure walker)
 *
 *  Responsible for asking the Linux kernel for the raw SMBIOS table
 *  (via /sys/firmware/dmi/tables/DMI and .../smbios_entry_point) and
 *  exposing a small iterator-style API so the display modules can
 *  pretty-print fields without having to think about struct boundaries
 *  or string tables.
 *
 *  Reference: DMTF DSP0134 -- "System Management BIOS (SMBIOS) Reference Spec"
 * =============================================================================
 */
#ifndef SMBIOS_H
#define SMBIOS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SMBIOS structure header. Every structure in the table starts with this. */
#pragma pack(push, 1)
typedef struct SmbiosHeader {
    uint8_t  type;     /* structure type (0=BIOS, 1=System, 2=Board, 4=CPU,
                          16=MemArray, 17=MemDevice, 127=End-of-table, ...) */
    uint8_t  length;   /* length of the FORMATTED section (header + fields).
                          Strings live AFTER this for `length` more bytes. */
    uint16_t handle;   /* unique identifier inside the table */
} SmbiosHeader;
#pragma pack(pop)

/* Wrapper around a loaded SMBIOS table. */
typedef struct SmbiosTable {
    uint8_t   smbiosMajorVersion;
    uint8_t   smbiosMinorVersion;
    uint8_t   dmiRevision;
    uint32_t  length;       /* size of the SMBIOS table data in bytes      */
    uint8_t  *tableData;    /* start of the first structure                */
    uint8_t  *rawBuffer;    /* original allocation -- freed by smbios_free */
    uint32_t  rawSize;
} SmbiosTable;

/* Acquire the SMBIOS table from the OS. Returns 0 on success, non-zero on
 * failure (sets errno-style code via *errOut if non-NULL). */
int  smbios_load(SmbiosTable *out, unsigned long *errOut);

/* Release any memory owned by the table. Safe to call on a zeroed struct. */
void smbios_free(SmbiosTable *tbl);

/* Iterate structures. Pass NULL as `current` to get the first one.
 * Returns NULL when the end-of-table marker (type 127) is reached. */
const SmbiosHeader *smbios_next(const SmbiosTable *tbl,
                                const SmbiosHeader *current);

/* Find the first structure of a given type. NULL if not present. */
const SmbiosHeader *smbios_find(const SmbiosTable *tbl, uint8_t type);

/* Look up the Nth string (1-based) attached to a structure.
 * Returns "" for index 0 (spec: "no string"), "<bad index>" if out of range,
 * or "<not specified>" if the string is empty / whitespace. */
const char *smbios_string(const SmbiosHeader *hdr, uint8_t index);

/* Length helpers */
const uint8_t *smbios_struct_end(const SmbiosHeader *hdr);

#ifdef __cplusplus
}
#endif

#endif /* SMBIOS_H */
