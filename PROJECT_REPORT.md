# DMI / SMBIOS Hardware Explorer -- Project Report

**Course:** Operating Systems
**Authors:** SAHIN Leyla-Kubra and 
**Language:** C (C11)
**Target OS:** Linux (built and run on WSL2 Ubuntu)

---

## 1. Goal

Build a small user-space tool that asks the operating system for the
SMBIOS / DMI table -- the structured description of the hardware that the
firmware (BIOS / UEFI) hands to the OS at boot -- and presents the
information through a console interface, with a particular focus on the
**motherboard** and the **RAM slots** as required by the assignment.

## 2. Background

SMBIOS (System Management BIOS) is defined by the DMTF in document
DSP0134. Each SMBIOS *structure* starts with a fixed header (type, length,
handle) followed by a "formatted area" of typed fields and a
null-separated string table. The Linux kernel exposes the firmware's
copy of this table in `sysfs`:

```
/sys/firmware/dmi/tables/DMI                  -- raw structure stream
/sys/firmware/dmi/tables/smbios_entry_point   -- header with SMBIOS version
```

Our program opens those two files, parses what it finds, and walks the
structures. No external tools required, no `dmidecode` dependency --
this is the same data `dmidecode` reads, parsed by our own code.

> ### Note on WSL2
>
> The project was developed and is intended to be run on WSL2 Ubuntu.
> WSL2 is itself a Hyper-V virtual machine, so the SMBIOS table reflects
> the **virtual machine's** hardware as reported by Microsoft's
> hypervisor -- not the underlying laptop. For the demo, this means
> the motherboard field shows `Microsoft Corporation / Virtual Machine`
> and the memory array shows a single synthetic Hyper-V memory device.
> The parsing code itself is identical to what would run on bare-metal
> Linux; only the data source differs.

## 3. Architecture

The code is split across five compilation units, separating data
acquisition, structure walking, formatting helpers, and the two display
domains (system-side and memory-side):

```
                     +-------------------+
   main.c  --------> |  display.h API    |
   (menu loop)       +---------+---------+
                               |
            +------------------+-------------------+
            |                                      |
   display_system.c                       display_memory.c
   (Type 0, 1, 2, 4)                      (Type 16, 17)
            \                                      /
             \                                    /
              +------> util.h / util.c <---------+
                       (printing + enum lookup)
                              |
                              v
                       smbios.h / smbios.c
                       (sysfs DMI loader
                        + structure walker)
```

**Why this shape?** Each module has one job and one author. The loader
sits behind a narrow API so the rest of the code never has to think
about firmware specifics. The two display modules can be written and
tested in parallel because they only depend on the SMBIOS walker.

## 4. Author contributions

| File                      | Primary author    | Responsibility                          |
| ------------------------- | ----------------- | --------------------------------------- |
| `src/main.c`              | **SAHIN Leyla-Kubra**         | Menu loop, banner, dispatch             |
| `src/display_system.c`    | **SAHIN Leyla-Kubra**         | Type 0 / 1 / 2 / 4 pretty-printers      |
| `src/display.h`           | **SAHIN Leyla-Kubra**         | Shared display API                      |
| `src/smbios.h`            | **BOINEGRI Stefania-Denisa**     | Public parser API                       |
| `src/smbios.c`            | **BOINEGRI Stefania-Denisa**     | sysfs loader, structure walker          |
| `src/display_memory.c`    | **BOINEGRI Stefania-Denisa**     | Type 16 / 17 RAM-slot printers          |
| `src/util.h` / `util.c`   | **BOINEGRI Stefania-Denisa**     | Section headers, enum -> string tables  |
| `Makefile`                | shared            | Build script                            |
| `.vscode/*`               | **SAHIN Leyla-Kubra**         | IDE / debugger config (Remote-WSL)      |
| `README.md`               | **SAHIN Leyla-Kubra**         | Setup + usage docs                      |
| `PROJECT_REPORT.md`       | shared            | This document                           |

We pair-reviewed each other's modules and ran the program together on
both authors' WSL2 environments to confirm consistent output.

## 5. Key implementation details

**Reading the table.** `smbios_load()` opens
`/sys/firmware/dmi/tables/smbios_entry_point` to learn the SMBIOS
version (looking for the `_SM_` or `_SM3_` anchor at the start of the
file), then reads the entire `/sys/firmware/dmi/tables/DMI` file into a
malloc'd buffer. Both files are produced by the kernel's DMI subsystem
during boot.

**Walking the SMBIOS table.** Each structure is laid out as
`[ formatted area : header.length bytes ][ string1\0string2\0...\0\0 ]`.
The next structure starts immediately after the double-null terminator
that ends the string table. `smbios_struct_end()` finds that boundary;
`smbios_next()` is the public iterator.

**Defensive field access.** Older firmware can produce shorter
structures than the latest spec, so every field read is guarded by
`length >= required_offset`. Helpers `rb`, `rw`, `rd` (read byte / word /
dword) return zero past the end-of-formatted-area instead of reading
into the string table.

**Memory size encoding.** Type 17 stores size in a 16-bit field where
bit 15 selects KB vs MB. Modules larger than ~32 GB use the special
value `0x7FFF` and store the real size in a 32-bit *Extended Size* field
introduced in SMBIOS 2.7. We handle all three cases.

**Memory type enumeration.** The Memory Type byte (offset 0x12 in
Type 17) is decoded against the SMBIOS spec table -- 0x18 = DDR3,
0x1A = DDR4, 0x22 = DDR5, 0x1E = LPDDR4, etc. This is the field most
students focus on, so it has the largest lookup table.

## 6. What we learned

- **OS / firmware boundary.** The firmware lays down a binary blob; the
  kernel just hands it through `sysfs`. It's a nice illustration of
  how thin the "OS" layer is for some kinds of system information --
  the OS doesn't interpret the table at all, it just exposes the file.
- **C struct packing matters.** Forgetting `#pragma pack(1)` on the
  SMBIOS header would silently produce wrong handle values on x64.
- **Strings live outside the struct.** It took a couple of tries to get
  the iterator right because the string table's length isn't recorded
  anywhere -- you have to scan for the double-null.

## 7. Possible extensions

- Decode Type 3 (System Enclosure / Chassis) and Type 9 (System Slot).
- Output JSON for piping into other tools.
- Run on bare-metal Linux (dual boot, lab machine) to see real hardware
  data instead of the WSL2 Hyper-V virtual machine values.
