import os
import json
import subprocess
from datetime import datetime
from django.shortcuts import render

BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../dmi_explorer'))
HR = '-' * 60

SECTION_ICONS = {
    'smbios table summary':   'bi-table',
    'system information':     'bi-display',
    'bios information':       'bi-cpu',
    'motherboard':            'bi-motherboard',
    'baseboard':              'bi-motherboard',
    'processor information':  'bi-cpu-fill',
    'memory array':           'bi-device-hdd',
    'memory devices':         'bi-memory',
    'ram':                    'bi-memory',
}

_MEMORY_TYPE = {
    0:'Unknown',1:'Other',2:'DRAM',3:'Sync DRAM',17:'SDRAM',20:'DDR',
    21:'DDR2',22:'DDR2 FB-DIMM',24:'DDR3',26:'DDR4',34:'LPDDR4',35:'LPDDR5',
}
_FORM_FACTOR = {
    0:'Unknown',7:'SIMM',8:'DIMM',12:'SODIMM',13:'SRIMM',
}
_MEM_ARRAY_USE = {1:'Other',2:'Unknown',3:'System Memory',4:'Video Memory'}
_MEM_ARRAY_ECC = {1:'Other',2:'Unknown',3:'None',4:'Parity',5:'Single-Bit ECC',6:'Multi-Bit ECC'}


def _icon_for(title):
    low = title.lower()
    for key, icon in SECTION_ICONS.items():
        if key in low:
            return icon
    return 'bi-info-circle'


_SHORT = {
    'system information':     'System',
    'bios information':       'BIOS',
    'motherboard':            'Motherboard',
    'baseboard':              'Motherboard',
    'processor information':  'Processor',
    'memory array summary':   'Memory Array',
    'memory devices':         'RAM Slots',
    'ram slots':              'RAM Slots',
    'smbios table summary':   'Table Summary',
}

def _short_title(title):
    low = title.lower()
    for key, short in _SHORT.items():
        if key in low:
            return short
    return title

def _section(title, rows):
    return {'title': title, 'short': _short_title(title), 'icon': _icon_for(title), 'rows': rows}


def _kv(key, val):
    return {'key': key, 'val': str(val) if val is not None else '', 'sub': False}


def _sub(label):
    return {'key': None, 'val': label, 'sub': True}


def _fmt_bytes(n):
    if n is None:
        return 'Unknown'
    n = int(n)
    if n >= 1 << 30:
        return f"{n >> 30} GB"
    if n >= 1 << 20:
        return f"{n >> 20} MB"
    return f"{n} bytes"


def _fmt_kb(n):
    if n is None:
        return 'Unknown'
    n = int(n)
    if n >= 1 << 20:
        return f"{n >> 20} GB"
    if n >= 1 << 10:
        return f"{n >> 10} MB"
    return f"{n} KB"


def _fmt_wmi_date(s):
    if not s:
        return ''
    try:
        return datetime.strptime(s[:8], '%Y%m%d').strftime('%Y-%m-%d')
    except ValueError:
        return s


# ── C binary path ────────────────────────────────────────────────────────────

def _run_binary():
    if not os.path.isfile(BINARY):
        return '', f'Binary not found at {BINARY}. Run "make" in the project root.', 1
    try:
        result = subprocess.run(
            [BINARY], input='7\n\n0\n',
            capture_output=True, text=True, timeout=10,
        )
        return result.stdout, result.stderr, result.returncode
    except subprocess.TimeoutExpired:
        return '', 'The binary timed out after 10 seconds.', 1


def _parse_binary_output(output):
    lines = output.splitlines()
    sections = []
    current = None
    i = 0
    while i < len(lines):
        if lines[i].strip() == HR and i + 2 < len(lines) and lines[i + 2].strip() == HR:
            title = lines[i + 1].strip()
            if current is not None:
                sections.append(current)
            current = _section(title, [])
            i += 3
            continue
        if current is not None:
            stripped = lines[i].strip()
            if not stripped:
                i += 1
                continue
            if ':' in stripped and not stripped.startswith('--'):
                key, _, val = stripped.partition(':')
                current['rows'].append(_kv(key.strip(), val.strip()))
            else:
                current['rows'].append(_sub(stripped))
        i += 1
    if current is not None:
        sections.append(current)
    return sections


# ── Windows WMI fallback (WSL2) ───────────────────────────────────────────────

_PS_QUERY = (
    "$ErrorActionPreference='SilentlyContinue';"
    "@{"
    "BIOS=Get-WmiObject Win32_BIOS|Select Manufacturer,SMBIOSBIOSVersion,ReleaseDate,SerialNumber,Version;"
    "System=Get-WmiObject Win32_ComputerSystem|Select Manufacturer,Model,SystemFamily,TotalPhysicalMemory,NumberOfProcessors,SystemType,DNSHostName;"
    "BaseBoard=Get-WmiObject Win32_BaseBoard|Select Manufacturer,Product,Version,SerialNumber,Description;"
    "Processor=Get-WmiObject Win32_Processor|Select Name,Manufacturer,MaxClockSpeed,CurrentClockSpeed,NumberOfCores,NumberOfLogicalProcessors,SocketDesignation,ProcessorId;"
    "Memory=Get-WmiObject Win32_PhysicalMemory|Select BankLabel,DeviceLocator,Capacity,Speed,ConfiguredClockSpeed,Manufacturer,PartNumber,SerialNumber,MemoryType,FormFactor;"
    "MemArray=Get-WmiObject Win32_PhysicalMemoryArray|Select MaxCapacity,MemoryDevices,Use,ECC"
    "}|ConvertTo-Json -Depth 3"
)


def _run_wmi():
    try:
        result = subprocess.run(
            ['powershell.exe', '-Command', _PS_QUERY],
            capture_output=True, text=True, timeout=20,
        )
        if result.returncode != 0 or not result.stdout.strip():
            return None
        return json.loads(result.stdout)
    except Exception:
        return None


def _parse_wmi(data):
    sections = []

    # ── System Information ──
    s = data.get('System') or {}
    total = _fmt_bytes(s.get('TotalPhysicalMemory'))
    sections.append(_section('System Information', [
        _kv('Manufacturer',          s.get('Manufacturer')),
        _kv('Model',                 s.get('Model')),
        _kv('Family',                s.get('SystemFamily')),
        _kv('System Type',           s.get('SystemType')),
        _kv('Hostname',              s.get('DNSHostName')),
        _kv('Total Physical Memory', total),
        _kv('Processor Sockets',     s.get('NumberOfProcessors')),
    ]))

    # ── BIOS Information ──
    b = data.get('BIOS') or {}
    sections.append(_section('BIOS Information', [
        _kv('Vendor',         b.get('Manufacturer')),
        _kv('BIOS Version',   b.get('SMBIOSBIOSVersion')),
        _kv('Release Date',   _fmt_wmi_date(b.get('ReleaseDate'))),
        _kv('Serial Number',  b.get('SerialNumber')),
    ]))

    # ── Motherboard ──
    bb = data.get('BaseBoard') or {}
    sections.append(_section('Motherboard / Baseboard', [
        _kv('Manufacturer',  bb.get('Manufacturer')),
        _kv('Product',       bb.get('Product')),
        _kv('Version',       bb.get('Version')),
        _kv('Serial Number', bb.get('SerialNumber')),
        _kv('Description',   bb.get('Description')),
    ]))

    # ── Processor ──
    procs = data.get('Processor') or []
    if isinstance(procs, dict):
        procs = [procs]
    proc_rows = []
    for idx, p in enumerate(procs):
        if idx > 0:
            proc_rows.append(_sub(f'-- CPU #{idx + 1} --'))
        proc_rows += [
            _kv('Name',            p.get('Name', '').strip()),
            _kv('Manufacturer',    p.get('Manufacturer')),
            _kv('Socket',          p.get('SocketDesignation')),
            _kv('Max Speed',       f"{p.get('MaxClockSpeed')} MHz" if p.get('MaxClockSpeed') else ''),
            _kv('Current Speed',   f"{p.get('CurrentClockSpeed')} MHz" if p.get('CurrentClockSpeed') else ''),
            _kv('Core Count',      p.get('NumberOfCores')),
            _kv('Thread Count',    p.get('NumberOfLogicalProcessors')),
            _kv('Processor ID',    p.get('ProcessorId')),
        ]
    sections.append(_section('Processor Information', proc_rows))

    # ── Memory Array ──
    ma = data.get('MemArray') or {}
    if isinstance(ma, list):
        ma = ma[0] if ma else {}
    sections.append(_section('Memory Array Summary', [
        _kv('Maximum Capacity', _fmt_kb(ma.get('MaxCapacity'))),
        _kv('Memory Devices',   ma.get('MemoryDevices')),
        _kv('Use',              _MEM_ARRAY_USE.get(ma.get('Use'), 'Unknown')),
        _kv('ECC Type',         _MEM_ARRAY_ECC.get(ma.get('ECC'), 'Unknown')),
    ]))

    # ── RAM Slots ──
    slots = data.get('Memory') or []
    if isinstance(slots, dict):
        slots = [slots]
    slot_rows = []
    populated = 0
    total_bytes = 0
    for slot in slots:
        cap = slot.get('Capacity') or 0
        if cap:
            populated += 1
            total_bytes += int(cap)
        slot_rows.append(_sub(f"Slot: {slot.get('DeviceLocator', '?')}  ({slot.get('BankLabel', '')})"))
        if cap:
            slot_rows += [
                _kv('Size',              _fmt_bytes(cap)),
                _kv('Manufacturer',      (slot.get('Manufacturer') or '').strip()),
                _kv('Part Number',       (slot.get('PartNumber') or '').strip()),
                _kv('Serial Number',     (slot.get('SerialNumber') or '').strip()),
                _kv('Rated Speed',       f"{slot['Speed']} MHz" if slot.get('Speed') else ''),
                _kv('Configured Speed',  f"{slot['ConfiguredClockSpeed']} MHz" if slot.get('ConfiguredClockSpeed') else ''),
                _kv('Memory Type',       _MEMORY_TYPE.get(slot.get('MemoryType'), 'Unknown')),
                _kv('Form Factor',       _FORM_FACTOR.get(slot.get('FormFactor'), 'Unknown')),
            ]
        else:
            slot_rows.append(_kv('Status', 'Empty'))
    slot_rows.append(_sub(f'Summary: {populated}/{len(slots)} slots populated — {_fmt_bytes(total_bytes)} total'))
    sections.append(_section('RAM Slots / Modules', slot_rows))

    return sections


# ── View ──────────────────────────────────────────────────────────────────────

def about(request):
    return render(request, 'explorer/about.html')


def index(request):
    stdout, stderr, returncode = _run_binary()

    error = None
    sections = []
    source = 'smbios'

    if returncode == 0:
        sections = _parse_binary_output(stdout)
    else:
        wmi = _run_wmi()
        if wmi:
            sections = _parse_wmi(wmi)
            source = 'wmi'
        else:
            error = (stderr or stdout).strip()

    return render(request, 'explorer/index.html', {
        'sections':     sections,
        'error':        error,
        'source':       source,
        'binary_path':  BINARY,
    })
