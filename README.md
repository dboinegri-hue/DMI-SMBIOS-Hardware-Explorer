# DMI / SMBIOS Hardware Explorer

A C program that reads the **SMBIOS (DMI)** firmware table from the Linux kernel
and lets the user browse deep hardware information -- motherboard, BIOS, CPU, and
every RAM slot -- through a **console menu** and a **Django web interface**.

> Operating Systems course project.
> Built and run on **WSL2 Ubuntu**.
> Authors: **SAHIN Leyla-Kubra** and **BOINEGRI Stefania-Denisa**.

---

## What is SMBIOS / DMI?

When a computer boots, the BIOS/UEFI firmware fills in a structured blob called
the **SMBIOS table** (specified by the DMTF in document DSP0134). It describes
the hardware as the firmware sees it -- board manufacturer, serial numbers, every
populated and empty memory slot, the BIOS version, the CPU socket, and so on.

The Linux kernel exposes that blob to user-space at:

```
/sys/firmware/dmi/tables/DMI                  -- raw structure stream
/sys/firmware/dmi/tables/smbios_entry_point   -- header with SMBIOS version
```

> ### Heads-up about WSL2
>
> Some WSL2 kernels are compiled without `CONFIG_DMI_SYSFS`, so
> `/sys/firmware/dmi/tables/` does not exist even though the kernel has already
> processed the SMBIOS table at boot. The **web interface** works around this
> automatically by falling back to Windows WMI via `powershell.exe` interop.
> The console binary still requires the sysfs path; run it on bare-metal Linux
> or a VM with DMI passthrough for full console functionality.

---

## Project structure

```
DMI_SMBIOS_Explorer/
├── src/
│   ├── main.c                    Menu / entry point        (Leyla)
│   ├── smbios.h / smbios.c       /sys/firmware/dmi loader  (Stefania)
│   ├── display.h                 Shared display API
│   ├── display_system.c          Type 0/1/2/4 printers     (Leyla)
│   ├── display_memory.c          Type 16/17 printers       (Stefania)
│   └── util.h / util.c           Helpers + enum lookups    (Stefania)
├── web/                          Django web interface
│   ├── manage.py
│   ├── requirements.txt
│   ├── dmi_web/                  Django project config
│   │   ├── settings.py
│   │   ├── urls.py
│   │   └── wsgi.py
│   └── explorer/                 Django app
│       ├── views.py              Runs C binary; WMI fallback
│       ├── urls.py
│       └── templates/explorer/
│           ├── base.html         Shared layout (sakura theme, petals, navbar)
│           ├── index.html        Hardware data page
│           └── about.html        About / project info page
├── Dockerfile
├── docker-compose.yml
├── Makefile
├── README.md
└── PROJECT_REPORT.md
```

---

## Option A — Run with Docker (easiest, works on any laptop)

This is the recommended way for anyone who just wants to run the project without
setting up a C compiler or Python manually. Docker handles everything.

### Step 1 — Install Docker Desktop

Download and install Docker Desktop for your operating system:

- **Windows:** https://docs.docker.com/desktop/install/windows-install/
  - During install, make sure **"Use WSL 2 instead of Hyper-V"** is checked.
  - After install, open Docker Desktop and wait until it says **"Engine running"**
    in the bottom-left corner before continuing.

- **macOS:** https://docs.docker.com/desktop/install/mac-install/
  - Download the correct version for your chip (Apple Silicon or Intel).
  - Drag Docker to Applications, open it, and wait for it to finish starting.

- **Linux:** https://docs.docker.com/desktop/install/linux-install/

### Step 2 — Enable WSL integration (Windows only)

If you are on Windows, Docker needs permission to talk to your Ubuntu terminal.

1. Open **Docker Desktop**
2. Click the **gear icon** (Settings) in the top right
3. Go to **Resources → WSL Integration**
4. Turn on the toggle next to your **Ubuntu** distro
5. Click **Apply & Restart**

### Step 3 — Fix Docker socket permissions (WSL2 only, one time)

After the restart, open your **Ubuntu terminal** and run:

```bash
sudo chmod 666 /var/run/docker.sock
```

This lets your user account talk to Docker without `sudo`.
You only need to do this once per WSL2 session (it resets when Docker Desktop restarts).

To make it permanent, add yourself to the docker group:

```bash
sudo usermod -aG docker $USER
```

Then fully close all WSL2 terminals and reopen one — after that you will never
need the `chmod` step again.

### Step 4 — Get the project

Clone the repository (recommended) or copy the project folder onto your machine.

**Clone from GitHub:**

```bash
git clone https://github.com/leylakubrasahin/DMI_SMBIOS_Explorer.git
cd DMI_SMBIOS_Explorer
```

**Or, if you have the folder as a zip**, extract it and navigate to it.

On **Windows**, open the folder in your Ubuntu terminal via WSL2:

```bash
# Replace the path with wherever you put the project
cd /mnt/c/Users/YourName/Desktop/DMI_SMBIOS_Explorer
```

On **macOS / Linux**, just `cd` into the folder:

```bash
cd ~/DMI_SMBIOS_Explorer
```

### Step 5 — Build and run

Inside the project folder, run:

```bash
docker compose up --build
```

You will see Docker downloading a base image, compiling the C code, and
installing Python packages. **This takes about 1–2 minutes the first time.**
Once you see this line:

```
web-1  | Watching for file changes with StatReloader
```

the server is ready. Open **http://localhost:8000** in your browser.

### Step 6 — Stop the server

Press **Ctrl + C** in the terminal to stop it, or run in another terminal:

```bash
docker compose down
```

---

### Docker command reference

| Command | What it does |
| ------- | ------------ |
| `docker compose up --build` | Build the image and start (use this the first time, or after changing code) |
| `docker compose up` | Start without rebuilding (faster, for subsequent runs) |
| `docker compose up -d` | Start in the background (terminal stays free) |
| `docker compose down` | Stop the container and clean up |
| `docker compose logs -f` | See live server logs |

---

### What data will I see?

| Where you run it | What you get |
| ---------------- | ------------ |
| Linux bare-metal (in Docker) | Full real hardware data read directly from SMBIOS |
| Windows / macOS (in Docker) | Error page — the VM boundary blocks SMBIOS access |
| Windows WSL2 (without Docker) | Real hardware data via the Windows WMI fallback |

> **Note for Windows / macOS users:** Docker Desktop runs containers inside a
> lightweight Linux VM that cannot reach the host machine's firmware tables.
> The site will show an error card explaining this. If you want to see real
> hardware data, follow **Option B** below (WSL2 direct) instead.

---

## Option B — Run directly in WSL2 (Windows, with real hardware data)

This method skips Docker and runs the Django server straight inside WSL2.
When the SMBIOS tables are not accessible, it automatically falls back to
querying your real Windows hardware via WMI — so you always see actual data.

### Step 1 — Install WSL2 + Ubuntu

Open **PowerShell as Administrator** and run:

```powershell
wsl --install -d Ubuntu
```

Reboot if prompted. On first launch pick any username and password.

Confirm it is version 2:

```powershell
wsl -l -v
```

If the VERSION column shows `1`, upgrade it:

```powershell
wsl --set-version Ubuntu 2
```

### Step 2 — Install build tools and Python

Open your **Ubuntu terminal** and run:

```bash
sudo apt update
sudo apt install -y build-essential gdb python3 python3-pip
```

### Step 3 — Install Django

```bash
pip3 install django --break-system-packages
```

Confirm it worked:

```bash
python3 -c "import django; print(django.__version__)"
```

### Step 4 — Get the project

Clone from GitHub (recommended) or copy the folder into the Linux filesystem.
Working from `~/` is much faster than from `/mnt/c/...`.

**Clone:**

```bash
git clone https://github.com/leylakubrasahin/DMI_SMBIOS_Explorer.git ~/os/DMI_SMBIOS_Explorer
cd ~/os/DMI_SMBIOS_Explorer
```

**Or copy from Windows:**

```bash
cp -r "/mnt/c/Users/YourName/Desktop/DMI_SMBIOS_Explorer" ~/os/DMI_SMBIOS_Explorer
cd ~/os/DMI_SMBIOS_Explorer
```

### Step 5 — Build the C binary

```bash
make
```

You should see gcc compile each `.c` file and produce `dmi_explorer`.

### Step 6 — Start the web server

```bash
cd web
python3 manage.py runserver
```

Open **http://127.0.0.1:8000** in your browser.

Keep the terminal open — closing it stops the server. Press **Ctrl + C** to stop.

---

## Web interface

The site has a **Japanese sakura pink theme** — soft petal-pink background, a
deep cherry-wood navbar, and 28 CSS-animated cherry blossom petals falling
continuously across the page.

### Features

- **Falling sakura petals** — 32 petals with random size, shade, speed, drift,
  and rotation so the animation never looks the same twice.
- **Sticky sidebar** — section links highlight automatically as you scroll.
- **Section cards** — each hardware category in its own card with a key-value table.
- **Data source badge** — top-right corner shows **SMBIOS Direct** or
  **Windows WMI** so it is always clear where the data came from.
- **About page** — visit `/about` for a full explanation of what the project does,
  how SMBIOS works, the tech stack, and the team.

### Sections displayed

| Section | SMBIOS Type | WMI fallback class |
| ------- | ----------- | ------------------ |
| System Information | Type 1 | `Win32_ComputerSystem` |
| BIOS Information | Type 0 | `Win32_BIOS` |
| Motherboard / Baseboard | Type 2 | `Win32_BaseBoard` |
| Processor Information | Type 4 | `Win32_Processor` |
| Memory Array Summary | Type 16 | `Win32_PhysicalMemoryArray` |
| RAM Slots / Modules | Type 17 | `Win32_PhysicalMemory` |

---

## Using the console program (Linux / WSL2 with SMBIOS access)

```bash
./dmi_explorer          # or: sudo ./dmi_explorer
```

```
============================================================
            DMI / SMBIOS  Hardware Explorer
            Operating Systems Project
            Authors: SAHIN Leyla-Kubra & BOINEGRI Stefania
============================================================
  Loaded SMBIOS v3.4  |  table size: 4231 bytes
============================================================

  ==================== MENU ====================
   [1]  System Information     (Type 1)
   [2]  BIOS Information       (Type 0)
   [3]  Motherboard Info       (Type 2)   <-- assignment focus
   [4]  Processor Information  (Type 4)
   [5]  Memory Array Summary   (Type 16)
   [6]  RAM Slots / Modules    (Type 17)  <-- assignment focus
   [7]  Show ALL of the above
   [8]  SMBIOS Table Summary
   [0]  Exit
  ==============================================
```

If you see `ERROR: could not read the SMBIOS firmware table`, run with sudo or:

```bash
sudo chmod a+r /sys/firmware/dmi/tables/DMI \
               /sys/firmware/dmi/tables/smbios_entry_point
```

---

## Cross-check against `dmidecode`

```bash
sudo apt install -y dmidecode
sudo dmidecode -t baseboard      # compare with menu option 3
sudo dmidecode -t memory         # compare with menu options 5 + 6
```

---

## License / academic use

Written for an Operating Systems class assignment. Re-use freely with attribution.

---

## Authors

| Name | Contributions |
| ---- | ------------- |
| **SAHIN Leyla-Kubra** | UI · console menu · system, BIOS, motherboard & CPU display |
| **BOINEGRI Stefania-Denisa** | SMBIOS parser core · RAM display · helpers & enums |
