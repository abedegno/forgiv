"""Build map/functions.tsv and map/data.tsv from the image, by dis2000 and flow analysis.

    python3 tools/mkmap.py            # write the map (names, kinds and notes already in it are kept)
    python3 tools/mkmap.py --dry      # analyse and report only

How (docs/notes.md has the layout):
1. The image's parts are fixed here (REGIONS): the application's .text, the ramfuncs load image
   (run at 0x8000), .cinit, the constants, the boot loader and its RAM copy, the CSM words.
2. Each code part is disassembled with dis2000 at its run address.
3. Function starts: LCR and LB targets, code addresses loaded as constants (MOVL XARn,#addr:
   task entries, interrupt handlers), code addresses in constant tables and .cinit records that
   are not switch tables, and the runtime library members tools/libfind.py places.
4. Each function's extent is its flow from the start (branches followed, calls fall through);
   it ends at the next start. Words between a function's flow end and the next start that are
   not 0xFFFF start a function no reference reaches ("unreferenced").
5. Switch tables (MOVL XAR7,#table ... LB *XAR7) are sized by the bound check before them.
6. RAM variables: direct (DP-relative) operands with the data page tracked through each
   function's flow, and RAM addresses loaded as constants.

A row the map already has under a name of its own (not sub_..., var_... and the like) keeps
that name, kind and notes when this script has only a generated name for it, so names given by
hand survive a rebuild. Names in NAMES below win."""
import argparse, collections, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316, libfind

# ---- the image's parts -------------------------------------------------------------------------
TEXT = (0x3E8004, 0x3F1BB7)              # application .text (from the entry stub) incl. the library
RAMF = (0x3F1BB7, 0x3F336F, 0x8000)      # ramfuncs: load start, load end, run start (main's memcpy)
CINIT = (0x3F336F, 0x3F3B45)             # .cinit records, the terminator and one more 0 word
CONST = (0x3F3B45, 0x3F3F36)             # .econst and .switch
BOOT = [(0x3F5000, 0x3F5004, 0x3F5000), (0x3F5004, 0x3F5226, 0x0400), (0x3F5226, 0x3F52D4, 0x3F5226)]
CSM = (0x3F7F80, 0x3F7FF6)               # CSM reserved words, must be 0 when the CSM is used
CODESTART = (0x3F7FF6, 0x3F7FF8, 0x3F7FF6)
PASSWORDS = (0x3F7FF8, 0x3F8000)
SPACES = [(TEXT[0], TEXT[1], TEXT[0]), RAMF] + BOOT + [CODESTART]

# Names. Meanings from my earlier Ghidra research notes on D316 (not published), checked
# against dis2000 here. Ghidra's function
# starts there are sometimes off: FUN_003F122D is this map's 0x3F122A.
NAMES = {
    0x3E8004: ('app_entry', 'entered by LB from the boot loader at 0x3F528A; calls start'),
    0x3F18BA: ('start', 'watchdog off (WDCR=0x68), then c_int00'),
    0x3F1855: ('main', 'called by _args_main; creates the seven RTOS tasks'),
    0x3E8008: ('task4', 'RTOS task 4, period 20 ms'),
    0x3EDD52: ('task0_state_machine', 'RTOS task 0, period 20 ms (11 ms while starting the converter)'),
    0x3EE6A4: ('task1', 'RTOS task 1, period 20 ms'),
    0x3EC8BF: ('task2_control', 'RTOS task 2, period 20 ms: control task, runs the voltage monitor'),
    0x3EBD1B: ('task3_measure', 'RTOS task 3, period 10 ms: measurements'),
    0x3EAABB: ('task5_arm_link', 'RTOS task 5, period 60 ms: UART link to the ARM, page handlers per frame'),
    0x3F09E0: ('task6_bms_link', 'RTOS task 6, period 40 ms: BMS Modbus master on SCI-B'),
    0x3EAC5D: ('CRC16', 'Modbus CRC, tables auchCRCHi/auchCRCLo'),
    0x3EAC73: ('arm_link_frame', 'builds the DSP->ARM frames (pages)'),
    0x3EB1A9: ('arm_page0', 'ARM link page 0 handler (table at 0x3F3E58)'),
    0x3EB348: ('arm_page1', 'ARM link page 1 handler: HR97/HR98 -> 0xD8DF/0xD8DE'),
    0x3EB52B: ('arm_page2', 'ARM link page 2 handler'),
    0x3EB6D6: ('arm_page3', 'ARM link page 3 handler'),
    0x3EB83A: ('arm_page4', 'ARM link page 4 handler: HR109 -> 0xD8E2, HR110 -> 0xD8C9, HR111/112'),
    0x3EB977: ('arm_page5', 'ARM link page 5 handler'),
    0x3EBAB2: ('arm_page6', 'ARM link page 6 handler: BMS request forwarded from the ARM'),
    0x3EBB98: ('arm_page7', 'ARM link page 7 handler'),
    0x3F0895: ('bms_parse_hr', 'FC3 reply parser (HR registers of the battery)'),
    0x3F09CE: ('bms_fc4_reply', 'FC4 reply passed to the ARM'),
    0x3F0BC4: ('bms_lost', 'battery lost: zero limits and SoC, set 0xCF15 bit 9'),
    0x3F0BEB: ('status_d903', 'copies the over-voltage bit (0xCF15 bit 6) into 0xD903 bit 1'),
    0x3EFADC: ('init_defaults', 'defaults: 0xD8DE=560, 0xD8DF=420, 0xD8E0=120, 0xD8C9=4'),
    0x3ED155: ('batt_voltage_monitor', 'mismatch, under- and over-voltage bits of 0xCF15; start permit 0xD52D'),
    0x3EE5E5: ('soft_start', 'start-up sequencer, runs while 0xD52D/0xD52E allow'),
    0x3EE5C3: ('any_fault', 'tests 0xCF15'),
    0x3EE7E8: ('select_state', 'picks the operating state from the fault words'),
    0x3EC908: ('fault_hold', 'clears 0xD52E while there is a fault'),
    0x3EC92F: ('force_charge_power', 'force charge at 300 W or more'),
    0x3EC96D: ('charge_taper_soc', 'charge power limit 0xD51A by SoC from 90%'),
    0x3ECA0A: ('current_limit_tapers', 'BMS current limits tapered by voltage -> 0xD526/0xD528, bounds 0xC212/0xC214'),
    0x3ECDE3: ('forced_charge_discharge', 'mode 0xD8CE'),
    0x3ECFB1: ('soc_floor', 'SoC floor blocks: 0xCF19 bits 9 and 11'),
    0x3ED0E0: ('ctl_step6', 'control task step'),
    0x3ED31B: ('ctl_step1', 'control task step'),
    0x3ED314: ('ctl_step2', 'control task step'),
    0x3ED2F4: ('ctl_step3', 'control task step'),
    0x3EC8D7: ('ctl_step9', 'control task step'),
    0x3ED38A: ('init_system', 'called first by main: DINT, InitSysCtrl, InitPieCtrl, InitPieVectTable, copies the ramfuncs to 0x8000, InitFlash, peripherals; puts timer2_isr in the INT14 vector'),
    0x3ED3FD: ('InitSysCtrl', 'TI F2806x_SysCtrl.c shape: watchdog off, calls Device_cal at 0x3D7C80'),
    0x3ED4B5: ('InitPieCtrl', 'TI F2806x_PieCtrl.c shape: PIE off, PIEIER/PIEIFR cleared'),
    0x3ED4D6: ('InitPieVectTable', 'TI F2806x_PieVect.c: copies PieVectTableInit+3 vectors (125) to 0x0D06, PIE on'),
    0x3ED919: ('sci_init', 'SCI-A and SCI-B 8N1, BRR 0x0124 (9600 baud at LSPCLK 22.5 MHz)'),
    0x3ED946: ('timer2_init', 'CPU timer 2, 1 ms'),
    0x3ED967: ('cpu_timer_config', '(timer, MHz, period us)'),
    0x3EEFC3: ('init_rating', 'sets 0xD400 at start-up'),
    0x3F11FD: ('timer2_isr', 'INT14: RTOS tick'),
    0x3F122A: ('scib_rx_isr', 'SCI-B receive interrupt: sci_rx(1)'),
    0x3F134E: ('sci_rx', '(port) receive into the ring buffer'),
    0x3F1322: ('ring_reset', ''),
    0x3F145F: ('ring_read', ''),
    0x3F1417: ('sci_tx', '(port) send from the buffer; port 1 = SCI-B, buffer 0xD9C0'),
    0x3F16CE: ('os_wait', 'RTOS: wait for an event'),
    0x3F1810: ('os_sched', 'RTOS: pick the next task and switch (TRAP #31)'),
    0x3F17E1: ('os_highest', 'RTOS: highest ready task from 0xD2EC'),
    0x3F19ED: ('os_task_create', 'RTOS: (id, period, entry)'),
    0x3F1A9D: ('os_tick', 'RTOS tick, from timer2_isr'),
    0x3F1A67: ('os_start_high', 'RTOS: restore the first task'),
    0x3F1A77: ('os_ctx_sw_isr', 'RTOS: TRAP #31 context switch'),
    0x3F19DA: ('os_start', 'RTOS: start, called last by main'),
    0x3F1BAD: ('os_cpu_sr_save', 'PUSH ST1; disable interrupts'),
    0x3F1BB1: ('os_cpu_sr_restore', ''),
    0x3F1BB7: ('current_controller', 'runs from RAM at 0x8000; clamps between 0xC214 and 0xC212'),
    0x3F2D57: ('fast_fault_check', 'runs from RAM at 0x91A0: fault check in the fast interrupt code'),
    0x3F2343: ('converter_mode', 'hands 0xC002 to the current controller'),
    0x3F5000: ('boot_update', 'boot loader: CSM unlock, copy its RAM part to 0x400, call it'),
    0x3F5226: ('boot_main', 'boot loader entry (from codestart): 0x3F3FFC == 0xAA starts the application'),
    0x3F7FF6: ('codestart', 'flash entry point: LB boot_main; section=codestart'),
    0x3F528C: ('csm_unlock', 'writes the CSM password keys, reads the password locations'),
    0x3F52BB: ('mem_copy', '(src, end, dst)'),
    0x3F52C4: ('boot_copy_run', 'copies 0x3F5004..0x3F5225 to 0x400 and calls it'),
}
NAMES.update({
    0x3F1A9A: ('os_int_ctx_sw', 'RTOS: drops two return addresses and joins os_ctx_sw_isr'),
    0x3F334E: ('InitFlash', 'TI example code: flash wait states (FlashRegs 0x0A80), runs from RAM'),
    0x3F336B: ('DSP28x_usDelay', 'TI DSP28x_usDelay.asm, runs from RAM'),
    0x3F0874: ('EMPTY_ISR', 'TI F2806x_DefaultIsr.c (name by its place after LUF_ISR)'),
})
ASM = {0x3F1A67, 0x3F1A77, 0x3F1A9A, 0x3F1BAD, 0x3F1BB1, 0x3F7FF6, 0x3F336B}

# The PIE vector table's slots (F2806x_PieVect.c / F2806x_DefaultIsr.c); PieVectTableInit at
# 0x3F3B46 holds TI's default handlers, which take the slot's name.
PIE_SLOTS = (['PIE_RESERVED'] * 13 + ['INT13_ISR', 'INT14_ISR', 'DATALOG_ISR', 'RTOSINT_ISR', 'EMUINT_ISR',
             'NMI_ISR', 'ILLEGAL_ISR'] + ['USER%d_ISR' % i for i in range(1, 13)] +
             ['ADCINT1_ISR', 'ADCINT2_ISR', 'rsvd_ISR', 'XINT1_ISR', 'XINT2_ISR', 'ADCINT9_ISR', 'TINT0_ISR', 'WAKEINT_ISR'] +
             ['EPWM%d_TZINT_ISR' % i for i in range(1, 9)] + ['EPWM%d_INT_ISR' % i for i in range(1, 9)] +
             ['ECAP1_INT_ISR', 'ECAP2_INT_ISR', 'ECAP3_INT_ISR', 'rsvd_ISR', 'rsvd_ISR', 'rsvd_ISR', 'HRCAP1_INT_ISR', 'HRCAP2_INT_ISR'] +
             ['EQEP1_INT_ISR', 'EQEP2_INT_ISR', 'rsvd_ISR', 'HRCAP3_INT_ISR', 'HRCAP4_INT_ISR', 'rsvd_ISR', 'rsvd_ISR', 'USB0_INT_ISR'] +
             ['SPIRXINTA_ISR', 'SPITXINTA_ISR', 'SPIRXINTB_ISR', 'SPITXINTB_ISR', 'MRINTA_ISR', 'MXINTA_ISR', 'rsvd_ISR', 'rsvd_ISR'] +
             ['DINTCH%d_ISR' % i for i in range(1, 7)] + ['rsvd_ISR'] * 2 +
             ['I2CINT1A_ISR', 'I2CINT2A_ISR'] + ['rsvd_ISR'] * 6 +
             ['SCIRXINTA_ISR', 'SCITXINTA_ISR', 'SCIRXINTB_ISR', 'SCITXINTB_ISR', 'ECAN0INTA_ISR', 'ECAN1INTA_ISR', 'rsvd_ISR', 'rsvd_ISR'] +
             ['rsvd_ISR', 'rsvd_ISR'] + ['ADCINT%d_ISR' % i for i in range(3, 9)] +
             ['CLA1_INT%d_ISR' % i for i in range(1, 9)] +
             ['XINT3_ISR'] + ['rsvd_ISR'] * 5 + ['LVF_ISR', 'LUF_ISR'])
assert len(PIE_SLOTS) == 128

DATA_NAMES = {
    0x3E8000: ('version', 'const', 'firmware version 316 (0x013C)'),
    0x3F3B46: ('PieVectTableInit', 'pievect', '128 vectors, copied into the PIE vector table by main\'s init'),
    0x3F3C46: ('auchCRCHi', 'const', 'Modbus CRC table, one byte per word'),
    0x3F3D46: ('auchCRCLo', 'const', 'Modbus CRC table, one byte per word'),
    0x3F3E58: ('arm_page_handlers', 'fptrs', 'ARM link page handlers 0..7'),
    0x3F3FFC: ('app_valid', 'const', '0xAA: the boot loader starts the application'),
    0x3F7F80: ('csm_rsvd', 'csm', 'CSM reserved words, all 0; section=csm_rsvd'),
    0x3F7FF8: ('csm_passwords', 'csm', 'CSM password locations; section=csmpasswds'),
}
VAR_NOTES = {
    0xD8E2: 'full (1) or short BMS poll; HR109 enable_bms_read via the ARM link',
    0xD8C9: 'SoC floor, HR110',
    0xD8C5: 'HR111 battery_charge_limit',
    0xD8C6: 'HR112 battery_discharge_limit',
    0xD8DE: 'battery maximum 0.1 V (HR98)',
    0xD8DF: 'battery minimum 0.1 V (HR97)',
    0xD8E0: 'maximum current?',
    0xD902: 'BMS valid reply seen',
    0xD907: 'battery calibration (HR29)',
    0xD915: 'ticks since last valid BMS reply',
    0xD916: 'BMS tick counter',
    0xD91C: 'BMS replies since loss (to 50)',
    0xCF15: 'fault word: bit 1 V mismatch, 2 UV, 6 OV, 9 BMS comms',
    0xCF19: 'blocks: bit 8 full, 9 force charge, 11 no discharge',
    0xD212: 'battery voltage (float, 0.1 V)',
    0xD504: 'BMS limit from HR26', 0xD505: 'BMS limit from HR27', 0xD506: 'SoC (HR21)',
    0xD501: 'HR19 status', 0xD508: 'pack voltage (HR22)', 0xD509: 'pack current (HR23)',
    0xD52D: 'start permit', 0xD52E: 'soft start allowed', 0xC002: 'converter mode',
    0xD526: 'tapered HR26 limit', 0xD528: 'tapered HR27 limit', 0xC212: 'charge bound', 0xC214: 'discharge bound',
    0xD892: 'rated power 0.1 W', 0xD400: 'rated power',
    0xDA80: 'RTOS task control blocks, 8 words per task',
    0xCFC0: 'ARM->DSP frame buffer', 0xD940: 'BMS reply buffer', 0xD9C0: 'SCI-B transmit buffer',
}


# ---- instructions ------------------------------------------------------------------------------
class Code:
    def __init__(self):
        self.ins = {}        # run address -> (nwords, text, load address)
        self.space = {}      # run address -> space index

    def add_space(self, i, lo, hi, run):
        for a, ws, t in d316.disasm(d316.words(lo, hi - lo), run, cache_key='space_%06X_%06X' % (lo, run)):
            self.ins[a] = (len(ws), t, a - run + lo)
            self.space[a] = i

    def load(self, run):
        return self.ins[run][2] if run in self.ins else None


def run_to_load(a):
    for lo, hi, run in SPACES:
        if run <= a < run + (hi - lo): return a - run + lo
    return None


def load_to_run(a):
    for lo, hi, run in SPACES:
        if lo <= a < hi: return a - lo + run
    return None


_REL = re.compile(r'^(SB|B|SBF|BF|BANZ|BAR|XBANZ)\s+(-?\d+)')
_ABS = re.compile(r'^(LB|LCR|LC|FFC)\s+(?:XAR7,\s*)?0x([0-9a-f]+)$')
ENDS = ('LRETR', 'IRET', 'LRET', 'LRETE', 'RETE')


def successors(a, n, t):
    """(next instructions, call targets, jump-away targets, indirect jump?)"""
    m = t.split()
    op = m[0] if m else ''
    if op in ENDS: return [], [], [], False
    r = _REL.match(t)
    if r:
        tgt = a + int(r.group(2))
        if t.rstrip().endswith('UNC'): return [tgt], [], [], False
        return [a + n, tgt], [], [], False
    r = _ABS.match(t)
    if r:
        tgt = int(r.group(2), 16)
        if op == 'LB': return [], [], [tgt], False
        return [a + n], [tgt], [], False
    if t.startswith('LB *'): return [], [], [], True
    return [a + n], [], [], False


# ---- analysis ----------------------------------------------------------------------------------
def analyse():
    code = Code()
    for i, (lo, hi, run) in enumerate(SPACES): code.add_space(i, lo, hi, run)
    ins = code.ins

    def in_code(a):
        return a in ins

    starts = {}                                  # run address -> set of reasons
    def add(a, why):
        starts.setdefault(a, set()).add(why)

    for a in (0x3E8004, 0x3F5000, 0x3F5226, 0x3F7FF6): add(a, 'entry')
    lib = libfind.find()[1]
    rts = {}
    for f in lib:
        if f['kind'] != 'text': continue
        # a C name first (abort before C$$EXIT), the float name first (expf before exp)
        syms = sorted(f['syms'] or [(0, '?')], key=lambda vn: (vn[0], '$' in vn[1], -len(vn[1]), vn[1]))
        for k, (v, n) in enumerate(syms):
            if k and v == syms[k - 1][0]: continue
            nxt = next((w for w, _ in syms[k + 1:] if w > v), f['size'])
            rts[f['addr'] + v] = dict(name=n, size=nxt - v, member=f['member'],
                                      alias=[m for w, m in syms if w == v and m != n])
            add(f['addr'] + v, 'rts')

    # references from all code
    consts = collections.defaultdict(set)        # code address loaded as a constant -> from
    calls = collections.defaultdict(set)
    switch_sites = []
    for a, (n, t, _) in ins.items():
        succ, cl, away, ind = successors(a, n, t)
        for c in cl:
            calls[c].add(a); add(c, 'call')
        for c in away: add(c, 'jump')
        m = re.match(r'^MOVL XAR(\d), #0x([0-9a-f]+)$', t)
        if m:
            v = int(m.group(2), 16)
            # code addresses as constants: only in the application's code (the boot loader's
            # constants are counts and RAM addresses that happen to fall in its RAM copy's range)
            if in_code(v) and code.space[v] in (0, 1) and code.space[a] in (0, 1): consts[v].add(a)
            if m.group(1) == '7' and CONST[0] <= v < CONST[1]:
                # a switch dispatch reads the table within the next few instructions and LB *XAR7
                b = a
                for _ in range(8):
                    b += ins[b][0] if b in ins else 1
                    if b in ins and ins[b][1].startswith('LB *XAR7'):
                        switch_sites.append((a, v)); break
    for v, frm in consts.items(): add(v, 'const')

    # constant tables: code addresses in 32-bit pairs, except in switch tables
    switch = {}
    for a, tbl in switch_sites:
        # the bound: CMPB AH,#n / CMP AH,#n before the dispatch -> n+1 entries
        # (the compiler's form: MOV AH,@AL; ADDB AH,#-first; CMPB AH,#count-1; branch away if higher)
        n = None
        # (or with AL: ADDB AL,#-first; CMPB AL,#count-1): a compare right after a subtract
        for b in range(a - 1, a - 80, -1):
            m = re.match(r'^CMPB? (A[HL]), #0x([0-9a-f]+)$', ins.get(b, (0, ''))[1])
            if m and ins.get(b - 1, (0, ''))[1].startswith('ADDB %s, #-' % m.group(1)):
                n = int(m.group(2), 16) + 1; break
        switch[tbl] = dict(site=a, entries=n)
    in_switch = set()
    for tbl, s in switch.items():
        for k in range(2 * (s['entries'] or 0)): in_switch.add(tbl + k)
    plain = set(range(0x3F3C46, 0x3F3E46))      # the CRC tables hold bytes, not addresses
    for f in lib:
        if f['kind'] == 'data': plain.update(range(f['addr'], f['addr'] + f['size']))
    for a in range(CONST[0] + (CONST[0] & 1), CONST[1] - 1, 2):
        if a in in_switch or a in plain: continue
        v = d316.W(a) | d316.W(a + 1) << 16
        if v not in ins: continue
        if TEXT[0] <= v < TEXT[1] or v >= 0x8000 and v < 0x8000 + RAMF[1] - RAMF[0]:
            add(v, 'table')
    # .cinit records holding code addresses
    cin = cinit_records()
    for r in cin:
        if r['n'] == 2:
            v = r['data'][0] | r['data'][1] << 16
            if in_code(v): add(v, 'cinit')

    # flow: extents, iterate with gap starts
    jumps = {}                                   # LB *XAR7 address -> case addresses
    for tbl, sw in switch.items():
        b = sw['site']
        while b in ins and not ins[b][1].startswith('LB *XAR7'): b += ins[b][0]
        sw['lb'] = b
        jumps[b] = [d316.W(tbl + 2 * k) | d316.W(tbl + 2 * k + 1) << 16 for k in range(sw['entries'] or 0)]

    def flow(s, limit):
        seen, work = set(), [s]
        bad = []
        while work:
            a = work.pop()
            if a in seen: continue
            if a >= limit or a < s: continue
            if a not in ins: bad.append(a); continue
            seen.add(a)
            n, t, _ = ins[a]
            succ, cl, away, ind = successors(a, n, t)
            work += succ + jumps.get(a, [])
        return seen, bad

    # calls into a library member other than at its symbols (ll_div's internal routine) are not starts
    inside = set()
    for f in lib:
        if f['kind'] == 'text': inside.update(range(f['addr'] + 1, f['addr'] + f['size']))
    for a in list(starts):
        if a in inside and a not in rts: del starts[a]
    funcs = {}
    for _ in range(50):
        order = sorted(starts)
        new = False
        funcs = {}
        for k, s in enumerate(order):
            if s not in ins:
                continue
            sp = code.space[s]
            lo, hi, run = SPACES[sp]
            limit = min(order[k + 1] if k + 1 < len(order) else 1 << 30, run + hi - lo)
            if s in rts:
                end = s + rts[s]['size']
                seen = set(); bad = []
            else:
                seen, bad = flow(s, limit)
                end = max(a + ins[a][0] for a in seen) if seen else s + 1
            funcs[s] = dict(start=s, end=end, limit=limit, bad=bad, seen=seen)
            # words after the flow end, before the next start
            a = end
            while a < limit and d316.W(run_to_load(a)) == 0xFFFF: a += 1
            if a < limit and a not in starts:
                if a in ins:
                    add(a, 'gap'); new = True
        if not new: break
    return code, starts, funcs, rts, calls, consts, switch, cin, lib


def cinit_records():
    out = []
    p = CINIT[0]
    while True:
        c = d316.W(p)
        if c == 0: break
        if c & 0x8000:
            n, addr, h = 0x10000 - c, d316.W(p + 1) | d316.W(p + 2) << 16, 3
        else:
            n, addr, h = c, d316.W(p + 1), 2
        out.append(dict(at=p, n=n, addr=addr, size=h + n, data=d316.words(p + h, n)))
        p += h + n
    return out


# ---- RAM references with the data page tracked -------------------------------------------------
_DIRECT = re.compile(r'(?<![\w*\[(:#])@0x([0-9a-f]+)')
WIDE = ('MOVL', 'MOV32', 'ADDL', 'SUBL', 'CMPL', 'ADDUL', 'SUBUL', 'MOVDL', 'MOVA', 'MOVP', 'MOVS', 'SUBBL', 'ADDCL', 'SUBCUL')


def ram_refs(code, funcs):
    refs = collections.defaultdict(lambda: dict(n=0, wide=False, funcs=set()))
    ins = code.ins
    for s, f in funcs.items():
        seen = f['seen']
        if not seen: continue
        dp = {s: None}
        work = [s]
        state = {}
        while work:
            a = work.pop()
            if a not in seen: continue
            cur = dp.get(a)
            if a in state and state[a] == cur: continue
            state[a] = cur
            n, t, _ = ins[a]
            m = re.match(r'^MOVW DP, #0x([0-9a-f]+)$', t)
            out = cur
            if m: out = int(m.group(1), 16)
            elif t.startswith(('LCR', 'POP DP', 'MOV DP', 'MOVZ DP', 'FFC', 'TRAP', 'INTR')) or 'DP' in t.split(',')[0]:
                out = None
            succ, _, _, _ = successors(a, n, t)
            for b in succ:
                if b in dp and dp[b] != out:
                    if dp[b] is not None: dp[b] = None; work.append(b)
                elif b not in dp:
                    dp[b] = out; work.append(b)
        for a in seen:
            n, t, _ = ins[a]
            page = state.get(a)
            if t.startswith('MOVW DP'): continue
            for m in _DIRECT.finditer(t):
                if page is None: continue
                addr = page * 64 + int(m.group(1), 16)
                r = refs[addr]; r['n'] += 1; r['funcs'].add(s)
                if t.split()[0] in WIDE: r['wide'] = True
            m = re.match(r'^MOVL XAR\d, #0x([0-9a-f]+)$', t)
            if m:
                v = int(m.group(1), 16)
                # an address loaded as a constant: L0-L8 RAM, or a peripheral frame (smaller
                # numbers are mostly counts and limits, not addresses)
                in_frame = any(lo <= v < lo + n for lo, n, _ in FRAMES)
                if (0x8000 + RAMF[1] - RAMF[0] <= v < 0x14000 or in_frame) and v not in ins:
                    r = refs[v]; r['n'] += 1; r['funcs'].add(s); r['ptr'] = True
    return refs


# F2806x peripheral frames (TI's F2806x_Headers_nonBIOS.cmd): base, length, name
FRAMES = [(0x0880, 0x105, 'DevEmuRegs'), (0x0985, 3, 'SysPwrCtrlRegs'), (0x0A80, 8, 'FlashRegs'),
          (0x0AE0, 16, 'CsmRegs'), (0x0B00, 16, 'AdcResult'), (0x0C00, 8, 'CpuTimer0Regs'),
          (0x0C08, 8, 'CpuTimer1Regs'), (0x0C10, 8, 'CpuTimer2Regs'), (0x0CE0, 32, 'PieCtrlRegs'),
          (0x0D00, 256, 'PieVectTable'), (0x1400, 0x80, 'Cla1Regs'), (0x5000, 0x40, 'McbspaRegs'),
          (0x6000, 0x34, 'ECanaRegs'), (0x6100, 0x100, 'ECanaMboxes'), (0x6400, 0x20, 'Comp1Regs'),
          (0x6420, 0x20, 'Comp2Regs'), (0x6440, 0x20, 'Comp3Regs')] + \
         [(0x6800 + 0x40 * i, 0x40, 'EPwm%dRegs' % (i + 1)) for i in range(8)] + \
         [(0x6A00, 0x20, 'ECap1Regs'), (0x6A20, 0x20, 'ECap2Regs'), (0x6A40, 0x20, 'ECap3Regs'),
          (0x6AC0, 0x20, 'HRCap1Regs'), (0x6AE0, 0x20, 'HRCap2Regs'), (0x6B00, 0x40, 'EQep1Regs'),
          (0x6B40, 0x40, 'EQep2Regs'), (0x6C80, 0x20, 'HRCap3Regs'), (0x6CA0, 0x20, 'HRCap4Regs'),
          (0x6F80, 0x40, 'GpioCtrlRegs'), (0x6FC0, 0x20, 'GpioDataRegs'), (0x6FE0, 0x20, 'GpioIntRegs'),
          (0x7010, 0x20, 'SysCtrlRegs'), (0x7040, 0x10, 'SpiaRegs'), (0x7050, 0x10, 'SciaRegs'),
          (0x7060, 0x10, 'NmiIntruptRegs'), (0x7070, 0x10, 'XIntruptRegs'), (0x7100, 0x80, 'AdcRegs'),
          (0x7740, 0x10, 'SpibRegs'), (0x7750, 0x10, 'ScibRegs'), (0x7900, 0x40, 'I2caRegs')]


def periph_name(a):
    """(row name, note) for a peripheral address: a frame's base takes the frame's name"""
    for lo, n, name in FRAMES:
        if lo <= a < lo + n:
            return (name if a == lo else 'p_%04X' % a), ('%s+0x%X' % (name, a - lo) if a != lo else name)
    return 'p_%04X' % a, 'peripheral'


# ---- rows --------------------------------------------------------------------------------------
def build(code, starts, funcs, rts, calls, consts, switch, cin, lib, refs):
    ins = code.ins
    frows, drows = [], []
    lib_ranges = []
    pie = {}
    for k in range(128):
        a = 0x3F3B46 + 2 * k
        pie.setdefault(d316.W(a) | d316.W(a + 1) << 16, PIE_SLOTS[k])
    for s in sorted(funcs):
        f = funcs[s]
        load = run_to_load(s)
        end = f['end'] if s in rts else min(f['end'], f['limit'])
        size = end - s
        notes = []
        if s in rts:
            r = rts[s]
            kind, name = 'rts', r['name'].lstrip('_') if r['name'].startswith('_') and not r['name'].startswith('__') else r['name']
            if r['name'].startswith('__'): name = r['name'][1:]
            notes.append('member=' + r['member'])
            if r['alias']: notes.append('alias=' + ','.join(r['alias']))
        else:
            name, note = NAMES.get(load, ('sub_%06X' % load, ''))
            if s in pie and name.startswith('sub_'):
                name, note = pie[s], 'TI default handler (F2806x_DefaultIsr.c), PieVectTableInit slot'
            if note: notes.append(note)
            kind = 'asm' if load in ASM else 'c'
            if load != s: kind = 'ramfunc'
        if load != s: notes.append('run=0x%04X' % s)
        why = starts.get(s, set())
        if why == {'gap'}: notes.append('unreferenced')
        elif 'call' not in why and 'rts' not in why:
            notes.append('ref=' + ','.join(sorted(why)))
        if f['bad']: notes.append('flow leaves the code at ' + ','.join('0x%X' % b for b in f['bad'][:3]))
        if f['end'] > f['limit'] and s not in rts: notes.append('flow runs past the end')
        frows.append(dict(addr=load, size=size, name=name, kind=kind, notes='; '.join(notes)))
    # data rows
    for a, (name, kind, note) in DATA_NAMES.items():
        size = {0x3E8000: 1, 0x3F3B46: 256, 0x3F3C46: 256, 0x3F3D46: 256, 0x3F3E58: 16, 0x3F3FFC: 1,
                0x3F7F80: CSM[1] - CSM[0], 0x3F7FF8: 8}[a]
        drows.append(dict(addr=a, size=size, name=name, kind=kind, notes=note))
    for r in cin:
        drows.append(dict(addr=r['at'], size=r['size'], name='cinit_%06X' % r['at'], kind='cinit',
                          notes='var=0x%04X n=%d' % (r['addr'], r['n'])))
    end = cin[-1]['at'] + cin[-1]['size']
    drows.append(dict(addr=end, size=CINIT[1] - end, name='cinit_end', kind='cinit',
                      notes='two 0 words the linker writes after the records (--rom_model)'))
    fnames = {d316.run_addr(r): r['name'] for r in frows}
    for tbl, s in sorted(switch.items()):
        owner = max((x for x in funcs if x <= s['site']), default=None)
        drows.append(dict(addr=tbl, size=2 * (s['entries'] or 0), name='sw_%s' % fnames.get(owner, 'X'),
                          kind='switch', notes='%d entries; dispatch at 0x%06X' % (s['entries'] or 0, s['site'])))
    for f in lib:
        if f['kind'] == 'data':
            drows.append(dict(addr=f['addr'], size=f['size'], name='%s_econst' % f['member'][:-4], kind='rts',
                              notes='member=%s section=%s' % (f['member'], f['section'])))
    # the rest of the constant area: function pointer tables and plain constants
    covered = set()
    for r in drows:
        covered.update(range(r['addr'], r['addr'] + r['size']))
    a = CONST[0]
    while a < CONST[1]:
        if a in covered or d316.W(a) == 0xFFFF and (a & 1) and a + 1 not in covered and False:
            a += 1; continue
        if a in covered: a += 1; continue
        b = a
        while b < CONST[1] and b not in covered: b += 1
        # split: runs of code pointers become fptr tables
        k = a
        while k < b:
            v = d316.W(k) | d316.W(k + 1) << 16 if k + 1 < b else -1
            if not (k & 1) and v in ins and k + 1 < b:
                j = k
                while j + 1 < b and (d316.W(j) | d316.W(j + 1) << 16) in ins: j += 2
                drows.append(dict(addr=k, size=j - k, name='fptrs_%06X' % k, kind='fptrs',
                                  notes='%d code addresses' % ((j - k) // 2)))
                k = j
            else:
                j = k
                while j < b and not (not (j & 1) and j + 1 < b and (d316.W(j) | d316.W(j + 1) << 16) in ins): j += 1
                if all(d316.W(x) == 0xFFFF for x in range(k, j)):
                    pass   # fill (alignment hole)
                else:
                    drows.append(dict(addr=k, size=j - k, name='c_%06X' % k, kind='const', notes=''))
                k = j
        a = b
    # RAM variables
    cvars = {}
    for r in cin: cvars[r['addr']] = r['n']
    for addr in sorted(set(refs) | set(cvars)):
        if addr >= 0x3D0000: continue          # reads of flash (the version word, the CSM passwords)
        r = refs.get(addr, dict(n=0, wide=False, funcs=set()))
        if 0x0800 <= addr < 0x8000:
            kind = 'periph'
            name, note = periph_name(addr)
        else:
            kind, name, note = 'var', 'var_%04X' % addr, ''
        size = cvars.get(addr) or (2 if r['wide'] else (1 if r['n'] and not r.get('ptr') else 0))
        frame = [n for lo, n, nm in FRAMES if lo == addr]
        if frame and kind == 'periph': size = frame[0]
        bits = []
        if note: bits.append(note)
        if addr in VAR_NOTES: bits.append(VAR_NOTES[addr])
        if r.get('ptr'): bits.append('address taken')
        if addr in cvars: bits.append('cinit')
        bits.append('refs=%d' % r['n'])
        drows.append(dict(addr=addr, size=size, name=name, kind=kind, notes='; '.join(bits)))
    return sorted(frows, key=lambda r: r['addr']), sorted(drows, key=lambda r: r['addr'])


AUTO = re.compile(r'^(sub|var|p|c|cinit|fptrs|sw)_')


def merge(old, new):
    """keep hand edits: where this script only has a generated name (sub_..., var_...) for a row
    and the map has another name, the map's name, kind and notes stay (names in NAMES win)"""
    o = {r['addr']: r for r in old}
    for r in new:
        p = o.get(r['addr'])
        if p and AUTO.match(r['name']) and not AUTO.match(p['name']):
            r['name'], r['kind'], r['notes'] = p['name'], p['kind'], p['notes']
    return new


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry', action='store_true')
    a = ap.parse_args()
    code, starts, funcs, rts, calls, consts, switch, cin, lib = analyse()
    refs = ram_refs(code, funcs)
    frows, drows = build(code, starts, funcs, rts, calls, consts, switch, cin, lib, refs)
    by = collections.Counter(r['kind'] for r in frows)
    print('functions:', len(frows), dict(by))
    for r in frows:
        if 'flow' in r['notes']: print('  check %06X %s: %s' % (r['addr'], r['name'], r['notes']))
    if a.dry: return
    os.makedirs(d316.MAPDIR, exist_ok=True)
    fp, dp = os.path.join(d316.MAPDIR, 'functions.tsv'), os.path.join(d316.MAPDIR, 'data.tsv')
    d316.write_tsv(fp, merge(d316.read_tsv(fp), frows), d316.FCOLS)
    d316.write_tsv(dp, merge(d316.read_tsv(dp), drows), d316.DCOLS)
    print('wrote', fp, dp)


if __name__ == '__main__':
    main()
