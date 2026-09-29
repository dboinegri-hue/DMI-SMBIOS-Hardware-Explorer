/* =============================================================================
 *  util.h -- Shared printing + lookup helpers
 *  -----------------------------------------------------------------------------
 *  Author : [Partner Name]
 *
 *  Small functions both display modules need: section headers, horizontal
 *  rules, and SMBIOS enum -> human string lookups (memory type, form factor,
 *  baseboard type, etc.). Keeping them here means display_system.c and
 *  display_memory.c don't have to duplicate any pretty-printing code.
 * =============================================================================
 */
#ifndef DMI_UTIL_H
#define DMI_UTIL_H

#include <stdint.h>
#include "smbios.h"

void util_hr(void);
void util_section(const char *title);
void util_kv(const char *key, const char *val);
void util_kv_u(const char *key, unsigned val, const char *unit);

/* SMBIOS spec enum lookups. All return a static string; never free. */
const char *util_baseboard_type(uint8_t code);
const char *util_processor_family(uint16_t code);
const char *util_processor_type(uint8_t code);
const char *util_memory_form_factor(uint8_t code);
const char *util_memory_type(uint8_t code);
const char *util_memory_array_location(uint8_t code);
const char *util_memory_array_use(uint8_t code);
const char *util_memory_ec_type(uint8_t code);

/* Pretty-print a memory size given in MiB, choosing GB/MB automatically.
 * Writes into `dst` (size `dstSize`) and returns dst. */
char *util_format_size_mib(uint32_t mib, char *dst, size_t dstSize);

#endif /* DMI_UTIL_H */
