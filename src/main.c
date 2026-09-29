/* =============================================================================
 *  main.c -- DMI / SMBIOS Hardware Explorer (entry point + menu)
 *  -----------------------------------------------------------------------------
 *  Project : DMI / SMBIOS Hardware Explorer
 *  Course  : Operating Systems
 *  Authors : Leyla            -- UI / menu / system+BIOS+motherboard+CPU display
 *            [Partner Name]   -- SMBIOS parser core, RAM display, helpers
 *
 *  WHAT THIS PROGRAM DOES
 *  ----------------------
 *  An SMBIOS (DMI) table is a structured blob that the system firmware
 *  (BIOS / UEFI) populates at boot describing the physical hardware --
 *  motherboard, CPU, memory slots, BIOS, and so on. The OS exposes that
 *  blob to user-space programs; on Linux it lives in
 *  /sys/firmware/dmi/tables/DMI.
 *
 *  This program asks the OS for that table, parses it according to the
 *  SMBIOS Reference Specification (DMTF DSP0134), and presents the fields
 *  through a small console menu so a user can browse motherboard info,
 *  walk every RAM slot, check BIOS version, and so on -- all without
 *  rebooting into firmware setup.
 *
 *  WHY IT'S USEFUL
 *  ---------------
 *  Demonstrates how the OS surfaces firmware-reported hardware metadata
 *  to user-space; touches Linux's /sys interface, structured binary
 *  parsing, string-table walking, and clean module separation.
 *
 *  BUILD
 *  -----
 *      Visual Studio Code  :  press Ctrl+Shift+B
 *      Command line (gcc)  :  see Makefile / README.md
 *
 *  On WSL2 / minimal installs you may need to run with sudo if
 *  /sys/firmware/dmi/tables/DMI isn't world-readable.
 * =============================================================================
 */
#include "smbios.h"
#include "display.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_banner(const SmbiosTable *tbl)
{
    puts("");
    puts("============================================================");
    puts("            DMI / SMBIOS  Hardware Explorer");
    puts("            Operating Systems Project");
    puts("            Authors: Leyla & [Partner Name]");
    puts("============================================================");
    printf("  Loaded SMBIOS v%u.%u  |  table size: %u bytes\n",
           tbl->smbiosMajorVersion, tbl->smbiosMinorVersion, tbl->length);
    puts("============================================================");
}

static void print_menu(void)
{
    puts("");
    puts("  ==================== MENU ====================");
    puts("   [1]  System Information     (Type 1)");
    puts("   [2]  BIOS Information       (Type 0)");
    puts("   [3]  Motherboard Info       (Type 2)   <-- assignment focus");
    puts("   [4]  Processor Information  (Type 4)");
    puts("   [5]  Memory Array Summary   (Type 16)");
    puts("   [6]  RAM Slots / Modules    (Type 17)  <-- assignment focus");
    puts("   [7]  Show ALL of the above");
    puts("   [8]  SMBIOS Table Summary");
    puts("   [0]  Exit");
    puts("  ==============================================");
    printf("  Choice: ");
    fflush(stdout);
}

static int read_choice(void)
{
    char buf[64];
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    int v = atoi(buf);
    return v;
}

static void pause_for_user(void)
{
    printf("\n  -- press Enter to return to menu -- ");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* drain */ }
}

int main(void)
{
    SmbiosTable tbl;
    unsigned long err = 0;
    int rc = smbios_load(&tbl, &err);
    if (rc != 0) {
        fprintf(stderr,
            "ERROR: could not read the SMBIOS firmware table.\n"
            "       smbios_load() returned %d, errno = %lu\n"
            "\n"
            "  Likely causes:\n"
            "    - /sys/firmware/dmi/tables/DMI doesn't exist on this system\n"
            "      (some container / chroot environments hide it).\n"
            "    - The DMI file isn't world-readable. Try:\n"
            "          sudo ./dmi_explorer\n"
            "      or, to make it readable for the current session:\n"
            "          sudo chmod a+r /sys/firmware/dmi/tables/DMI \\\n"
            "                         /sys/firmware/dmi/tables/smbios_entry_point\n",
            rc, err);
        return 1;
    }

    print_banner(&tbl);

    for (;;) {
        print_menu();
        int choice = read_choice();
        switch (choice) {
            case 1: display_system_info(&tbl);     pause_for_user(); break;
            case 2: display_bios_info(&tbl);       pause_for_user(); break;
            case 3: display_baseboard_info(&tbl);  pause_for_user(); break;
            case 4: display_processor_info(&tbl);  pause_for_user(); break;
            case 5: display_memory_array(&tbl);    pause_for_user(); break;
            case 6: display_memory_devices(&tbl);  pause_for_user(); break;
            case 7: display_all(&tbl);             pause_for_user(); break;
            case 8: display_table_summary(&tbl);   pause_for_user(); break;
            case 0:
                puts("\n  Goodbye!\n");
                smbios_free(&tbl);
                return 0;
            default:
                puts("  -- invalid choice, try again --");
                break;
        }
    }
}
