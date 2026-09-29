/* =============================================================================
 *  display.h -- Pretty-printers for individual SMBIOS structure types
 *  -----------------------------------------------------------------------------
 *  These functions take an already-loaded SmbiosTable and render the fields
 *  the user actually cares about (for an OS class assignment that's mostly
 *  motherboard + RAM, plus enough surrounding context to be useful).
 *
 *  Authors split:
 *      display_system.c   -- Leyla
 *      display_memory.c   -- [Partner Name]
 * =============================================================================
 */
#ifndef DMI_DISPLAY_H
#define DMI_DISPLAY_H

#include "smbios.h"

/* ---- system half (Leyla) ---------------------------------------------- */
void display_system_info(const SmbiosTable *tbl);     /* Type 1  */
void display_bios_info(const SmbiosTable *tbl);       /* Type 0  */
void display_baseboard_info(const SmbiosTable *tbl);  /* Type 2  -- Motherboard */
void display_processor_info(const SmbiosTable *tbl);  /* Type 4  */

/* ---- memory half (Partner) -------------------------------------------- */
void display_memory_array(const SmbiosTable *tbl);    /* Type 16 */
void display_memory_devices(const SmbiosTable *tbl);  /* Type 17 -- RAM slots */

/* Convenience: dump everything. */
void display_all(const SmbiosTable *tbl);

/* SMBIOS table summary (version, length, structure count). */
void display_table_summary(const SmbiosTable *tbl);

#endif /* DMI_DISPLAY_H */
