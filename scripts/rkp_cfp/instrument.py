#!/usr/bin/env python

# Copyright (c) 2016 Samsung Electronics Co., Ltd.
# Authors:	James Gleeson <jagleeso@gmail.com>
#		Wenbo Shen <wenbo.s@samsung.com>
#
# Instrument vmlinux STP, LDP and BLR instructions to protect RA and restrict jumpping
#
# Depends on:
# 1) a modified gcc that
#    - outputs 1 nop before function label
#    - outputs 1 nop before stp x29, x30 instructions
#    - outputs 1 nop after ldp x29, x30 instructions
# 2) a kernel built using gcc command-line options to prevent allocation of registers x16, and x17
#

import argparse
import subprocess
import common
import os
import re
import mmap
import contextlib
import binascii
import multiprocessing
import math
import tempfile
import pipes

# NOTE: must be kept in sync with macro definitions in init/hyperdrive.S
RRX_DEFAULT = 16
RRK_DEFAULT = 17

REG_FP = 29
REG_LR = 30
REG_SP = 31

DEFAULT_THREADS = multiprocessing.cpu_count()
#DEFAULT_THREADS = 1

BYTES_PER_INSN = 4

def bitmask(start_bit, end_bit):
    """
    e.g. start_bit = 8, end_bit = 2
    0b11111111  (2**(start_bit + 1) - 1)
    0b11111100  (2**(start_bit + 1) - 1) ^ (2**end_bit - 1)
    """
    return (2**(start_bit + 1) - 1) ^ (2**end_bit - 1);
def _zbits(x):
    """
    Return the number of low bits that are zero.
    e.g.
    >>> _zbits(0b11000000000000000000000000000000)
    30
    """
    n = 0
    while (x & 0x1) != 0x1 and x > 0:
        x >>= 1
        n += 1
    return n

# Use CROSS_COMPILE provided to kernel make command.
devnull = open('/dev/null', 'w')
def which(executable):
    return subprocess.Popen(['which', executable], stdout=devnull).wait() == 0

'''
Here comes the ugly part
For SLSI kernel, CROSS_COMPILE contains the whole path
For QC kernel, CROSS_COMPILE only have aarch64-linux-android-
'''
CROSS_COMPILE = os.environ.get('CROSS_COMPILE')
assert CROSS_COMPILE is not None

OBJDUMP = CROSS_COMPILE+"objdump"
NM = CROSS_COMPILE+"nm"

if (os.path.isfile(OBJDUMP) is False) and (not which(OBJDUMP)):
    raise RuntimeError(OBJDUMP+" does NOT contain full path and it is not in PATH"+"  PATH="+os.environ.get('PATH'))

if (os.path.isfile(NM) is False) and (not which(NM)):
    raise RuntimeError(NM+" does NOT contain full path and it is not in PATH"+"  PATH="+os.environ.get('PATH'))

hex_re = r'(?:[a-f0-9]+)'
virt_addr_re = re.compile(r'^(?P<virt_addr>{hex_re}):\s+'.format(hex_re=hex_re))

BL_OFFSET_MASK = 0x3ffffff
BLR_AND_RET_RN_MASK = 0b1111100000

ADRP_IMMLO_MASK = 0b1100000000000000000000000000000
ADRP_IMMHI_MASK =        0b111111111111111111100000
ADRP_RD_MASK    =                           0b11111
# _zbits
ADRP_IMMLO_ZBITS = _zbits(ADRP_IMMLO_MASK)
ADRP_IMMHI_ZBITS = _zbits(ADRP_IMMHI_MASK)
ADRP_RD_ZBITS = _zbits(ADRP_RD_MASK)


STP_OPC_MASK              = 0b11000000000000000000000000000000
STP_ADDRESSING_MODE_MASK  = 0b00111111110000000000000000000000
STP_IMM7_MASK             =           0b1111111000000000000000
STP_RT2_MASK              =                  0b111110000000000
STP_RN_MASK               =                       0b1111100000
STP_RT_MASK               =                            0b11111
# http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489c/CIHGJHED.html
# op{type}{cond} Rt, [Rn {, #offset}]        ; immediate offset
# op{type}{cond} Rt, [Rn, #offset]!          ; pre-indexed
# op{type}{cond} Rt, [Rn], #offset           ; post-indexed
# opD{cond} Rt, Rt2, [Rn {, #offset}]        ; immediate offset, doubleword
# opD{cond} Rt, Rt2, [Rn, #offset]!          ; pre-indexed, doubleword
STP_PRE_INDEXED =  0b10100110
STP_POST_INDEXED = 0b10100010
STP_IMM_OFFSET =   0b10100100
# opD{cond} Rt, Rt2, [Rn], #offset           ; post-indexed, doubleword
# 00 for 32bit, 10 for 64bit
OPC_32 = 0b00
OPC_64 = 0b10

# Bits don't encode preindexed for str_imm_unsigned_preindex_insn
STR_IMM_OFFSET = 'preindexed'
STR_SIGN_UNSIGNED = 0b01
STR_SIZE_32 = 0b10
STR_SIZE_64 = 0b11

ADDIM_OPCODE_BITS = 0b10001
ADDIMM_SF_BIT_64 = 0b1
ADDIMM_SF_BIT_32 = 0b0
ADDIMM_OPCODE_MASK = bitmask(28, 24)
ADDIMM_SHIFT_MASK = bitmask(23, 22)
ADDIMM_IMM_MASK = bitmask(21, 10)
ADDIMM_RN_MASK = bitmask(9, 5)
ADDIMM_RD_MASK = bitmask(4, 0)
ADDIMM_SF_MASK = bitmask(31, 31)

class NOPSTPClass(object):
    def __init__(self, curfunc='', good=False, loc=-1, insnfirst='', insnsecond=''):
        self.curfunc = curfunc
        self.good = good
        self.loc = loc
        self.insn1st = insnfirst
        self.insn2nd = insnsecond

nopStpInfo = NOPSTPClass()

def skip_func(func, skip, skip_asm):
    # Don't instrument the springboard itself.
    # Don't instrument functions in asm files we skip.
    # Don't instrument certain functions (for debugging).
    return func.startswith('jopp_springboard_') or \
           func in skip_asm or \
           func in skip

def parse_last_insn(objdump, i, n):
    return [objdump.parse_insn(j) if objdump.is_insn(j) else None for j in xrange(i-n, i)]

def instrument(objdump, func=None, skip=set([]), skip_stp=set([]), skip_asm=set([]), skip_blr=set([]), keep_magic=set([]), threads=1):
    """
    Replace:
        BLR rX
    With:
        BL jopp_springboard_blr_rX

    Replace
        <assembled_c_function>:
          nop
          stp	x29, x30, [sp,#-<frame>]!
          (insns)
    With:
        <assembled_c_function>:
          eor RRX, x30, RRK
          stp x29, RRX, [sp,#-<frame>]!
          (insns)

    Replace:
        ldp x29, x30, ...
        nop
    With:
        ldp x29, RRX, ...
        eor x30, RRX, RRK


    """
    def __instrument(func=None, start_func=None, end_func=None, start_i=None, end_i=None,
            tid=None):
        def parse_insn_range(i, r):
            return [objdump.parse_insn(j) if objdump.is_insn(j) else None for j in xrange(i, r)]

        #
        # Instrumentation of function prologues.
        #
        #import pdb; pdb.set_trace()
        def instrument_stp(curfunc, func_i, i, stp_insn, new_stp_insn, add_x29_imm):
            """
            new_stp_insn(stp_insn, replaced_insn) -> new stp instruction to encode.
            """
            last_insn = parse_insn_range(i-1, i)
            if not are_nop_insns(last_insn):
                return

            offset = insn['args']['imm']

            # eor RRX, x30, RRK
            eor = eor_insn(last_insn[0],
                    reg1=objdump.RRX, reg2=REG_LR, reg3=objdump.RRK)
            #objdump.write(i-1, objdump.encode_insn(eor))
            # stp x29, RRX, ...
            stp = new_stp_insn(insn, insn)
            #objdump.write(i, objdump.encode_insn(stp))
            global nopStpInfo
            nopStpInfo = NOPSTPClass(curfunc, True, i-1, objdump.encode_insn(eor), objdump.encode_insn(stp))

        def _skip_func(func):
            return skip_func(func, skip, skip_asm)

        last_func_i = [None]
        def each_insn():
            # Keep track of the last 2 instructions
            # (needed it for CONFIG_RKP_CFP_JOPP)
            for curfunc, func_i, i, insn, last_insns in objdump.each_insn(start_func=start_func, end_func=end_func,
                    start_i=start_i, end_i=end_i, skip_func=_skip_func, num_last_insns=1):
                yield curfunc, func_i, i, insn, last_insns
                last_func_i[0] = func_i

        for curfunc, func_i, i, insn, last_insns in each_insn():
            if objdump.JOPP and func_i != last_func_i[0] and are_nop_insns(ins[1] for ins in last_insns):
                # Instrument the nop just before the function.
                magic_i, magic_insn = last_insns[0]
                objdump.write(magic_i, objdump.JOPP_MAGIC)

            if objdump.JOPP and insn['type'] == 'blr' and curfunc not in skip_blr :
                springboard_blr = 'jopp_springboard_blr_x{register}'.format(register=insn['args']['dst_reg'])
                insn = bl_insn(insn,
                        offset=objdump.func_offset(springboard_blr) - objdump.insn_offset(i))
                objdump.write(i, objdump.encode_insn(insn))
                continue
            elif objdump.ROPP and curfunc not in skip_stp \
                    and insn['type'] == 'ldp' and \
                    insn['args']['reg1'] == REG_FP and \
                    insn['args']['reg2'] == REG_LR:
                forward_insn = parse_insn_range(i+1, i+2)
                if not are_nop_insns(forward_insn):
                    continue
                '''
                Note that for the new compiler, in same rare case
                nop is not immediate before stp x29, x30 or immediate
                after ldp x29, x30, so instrument BOTH only stp nop 
                and ldp nop are good
                '''
                global nopStpInfo
                if (nopStpInfo.curfunc != curfunc) or (nopStpInfo.good != True):
                    continue
                objdump.write(nopStpInfo.loc, nopStpInfo.insn1st)
                objdump.write(nopStpInfo.loc+1, nopStpInfo.insn2nd)

                # ldp x29, RRX, ...
                insn['args']['reg2'] = objdump.RRX
                ldp = ((hexint(insn['binary']) >> 15) << 15) | \
                       (insn['args']['reg2'] << 10) | \
                       ((insn['args']['base_reg']) << 5) | \
                       (insn['args']['reg1'])
                objdump.write(i, ldp)

                # eor x30, RRX, RRK
                eor = eor_insn(forward_insn[0],
                        reg1=REG_LR, reg2=objdump.RRX, reg3=objdump.RRK)
                objdump.write(i+1, objdump.encode_insn(eor))
                continue
            elif objdump.ROPP and curfunc not in skip_stp and insn['type'] == 'stp' and \
                    insn['args']['reg1'] == REG_FP and \
                    insn['args']['reg2'] == REG_LR and \
                    insn['args']['imm'] > 0:
                def stp_x29_RRX_offset(insn, replaced_insn):
                    # stp x29, RRX, [sp,#<offset>]
                    offset = insn['args']['imm']
                    return stp_insn(replaced_insn,
                        reg1=REG_FP, reg2=objdump.RRX, base_reg=REG_SP, imm=offset, mode=STP_IMM_OFFSET)
                instrument_stp(curfunc, func_i, i, insn, stp_x29_RRX_offset, insn['args']['imm'])
                continue
            elif objdump.ROPP and curfunc not in skip_stp and insn['type'] == 'stp' and \
                    insn['args']['reg1'] == REG_FP and \
                    insn['args']['reg2'] == REG_LR and \
                    insn['args']['imm'] < 0:
                def stp_x29_RRX_frame(insn, replaced_insn):
                    # stp x29, RRX, [sp,#-<frame>]!
                    frame = -1 * insn['args']['imm']
                    return stp_insn(replaced_insn,
                        reg1=REG_FP, reg2=objdump.RRX, base_reg=REG_SP, imm=-1 * frame, mode=STP_PRE_INDEXED)
                instrument_stp(curfunc, func_i, i, insn, stp_x29_RRX_frame, 0)
                continue
        objdump.flush()

    objdump.each_insn_parallel(__instrument, threads)

class Objdump(object):
    """
    Parse a vmlinux file, and apply instruction re-writes to a copy of it (or inplace).

    Makes heavy use of aarch64-linux-android-objdump output.
    i index in usage below is 0-based line number in aarch64-linux-android-objdump output.

    Usage:

    objdump = Objdump(vmlinux_filepath)
    objdump.parse()
    objdump.open()
    for i, insn in objdump.each_insn(func="stext"):
        if insn['type'] == 'bl':
            insn = bl_insn(insn, offset=32)
            objdump.write(i, objdump.encode_insn(insn))
    objdump.close()

    See "instrument" for implementation of actual hyperdrive instrumentations.
    """
    def __init__(self, vmlinux, config_file=None,
            RRK=RRK_DEFAULT, RRX=RRX_DEFAULT,
            instr="{dirname}/{basename}.instr", inplace=False,
            make_copy=True ):
        self.vmlinux = vmlinux
        self.vmlinux_old = None
        self.config_file = config_file
        self.conf = None
        self.c_functions = set([])
        self.lines = []
        self.func_idx = {}
        self.func_addrs = set([])
        self._funcs = None
        self.sections = None
        self.make_copy = make_copy

        #load config flags
        self._load_config()
        self.ROPP = self.is_conf_set('CONFIG_RKP_CFP_ROPP')
        self.JOPP = self.is_conf_set('CONFIG_RKP_CFP_JOPP')
        if self.JOPP:
            self.JOPP_MAGIC = int(self.get_conf('CONFIG_RKP_CFP_JOPP_MAGIC'), 16)

        self.RRK = RRK
        self.RRX = RRX

        self.instr_copy = None
        if inplace:
            self.instr = self.vmlinux
        else:
            basename = os.path.basename(vmlinux)
            dirname = my_dirname(vmlinux)
            if dirname == '':
                dirname = '.'
            self.instr = instr.format(**locals())



    def _load_config(self):
        if self.config_file:
            self.conf = parse_config(self.config_file)

    def parse(self):
        """
        Read and save all lines from "aarch64-linux-android-objdump -d vmlinux".
        Read and save section information from "aarch64-linux-android-objdump -x".
        Keep track of where in the objdump output functions occur.
        """
        self.sections = parse_sections(self.vmlinux)
        fd, tmp = tempfile.mkstemp()
        os.close(fd)

        subprocess.check_call("{OBJDUMP} -d {vmlinux} > {tmp}".format(
            OBJDUMP=OBJDUMP, vmlinux=pipes.quote(self.vmlinux), tmp=pipes.quote(tmp)), shell=True)

        # NOTE: DON'T MOVE THIS.
        # We are adding to the objdump output symbols from the data section.
        symbols = parse_nm(self.vmlinux)
        for s in symbols.keys():
            sym = symbols[s]
            if sym[NE_TYPE] in ['t', 'T']:
                self.func_addrs.add(_int(sym[NE_ADDR]))

        """
        Now process objdump output and extract information into self.lines:

        self.func_idx mapping from symbol name to a set of indicies into self.lines where
        that symbol is defined (there can be multiple places with the same symbol /
        function name in objdump).

        self.lines is tuple of:
        1. The line itself
        2. Which section each instructions occurs in
        3. Virtual addresses of instructions
        """
        section_idx = None
        with open(tmp, 'r') as f:
            for i, line in enumerate(f):
                virt_addr = None
                m = re.search(virt_addr_re, line)
                if m:
                    virt_addr = _int(m.group('virt_addr'))

                self.lines.append((line, section_idx, virt_addr))

                m = re.search(r'Disassembly of section (?P<name>.*):', line)
                if m:
                    section_idx = self.sections['section_idx'][m.group('name')]
                    continue

                m = re.search(common.fun_rec, line)
                if m:
                    if m.group('func_name') not in self.func_idx:
                        self.func_idx[m.group('func_name')] = set()
                    self.func_idx[m.group('func_name')].add(i)
                    continue
        # We have all the objdump lines read, we can delete the file now.
        #self._copy_to_tmp(tmp, 'objdump.txt')
        os.remove(tmp)

    def _copy_to_tmp(self, from_path, to_basename):
        """
        Copy file to dirname(vmlinux)/tmp/basename(filename)
        (e.g. tmp directory inside where vmlinux is)
        """
        vfile = os.path.join(my_dirname(self.vmlinux), 'scripts/rkp_cfp/tmp', to_basename)
        subprocess.check_call(['mkdir', '-p', my_dirname(vfile)])
        subprocess.check_call(['cp', from_path, vfile])
        return vfile

    def save_instr_copy(self):
        """
        Copy vmlinux_instr to dirname(vmlinux)/tmp/vmlinux.instr
        (mostly for debugging)
        """
        self.instr_copy = self._copy_to_tmp(self.instr, 'vmlinux.instr')

    def open(self):
        """
        mmap vmlinux for reading and vmlinux.instr for writing instrumented instructions.
        """
        if os.path.abspath(self.vmlinux) != os.path.abspath(self.instr):
            subprocess.check_call(['cp', self.vmlinux, self.instr])

        if self.make_copy:
            # copy vmlinux to tmp/vmlinux.old (needed for validate_instrumentation)
            self.vmlinux_old = self._copy_to_tmp(os.path.join(my_dirname(self.vmlinux), 'vmlinux'), 'vmlinux')
            self._copy_to_tmp(os.path.join(my_dirname(self.vmlinux), '.config'), '.config')

        self.write_f = open(self.instr, 'r+b')
        self.write_f.flush()
        self.write_f.seek(0)
        self.write_mmap = mmap.mmap(self.write_f.fileno(), 0, access=mmap.ACCESS_WRITE)

        self.read_f = open(self.vmlinux, 'rb')
        self.read_f.flush()
        self.read_f.seek(0)
        self.read_mmap = mmap.mmap(self.read_f.fileno(), 0, access=mmap.ACCESS_READ)

    def __getstate__(self):
        """
        For debugging.  Don't pickle non-picklable attributes.
        """
        d = dict(self.__dict__)
        del d['write_f']
        del d['write_mmap']
        del d['read_f']
        del d['read_mmap']
        del d['_funcs']
        return d
    def __setstate__(self, d):
        self.__dict__.update(d)

    def insn_offset(self, i):
        """
        Virtual address of
        """
        return self.parse_insn(i)['virt_addr']

    def _insn_idx(self, i):
        """
        Return the byte address into the file vmlinux.instr for the instruction at
        self.line(i).

        (this is all the index into an mmap of the file).
        """
        virt_addr = self.virt_addr(i)
        section_file_offset = self._section(i)['offset']
        section_virt = self._section(i)['address']
        return section_file_offset + (virt_addr - section_virt)

    def read(self, i, size=4):
        """
        Read a 32-bit instruction into a list of chars in big-endian.
        """
        idx = self._insn_idx(i)
        insn = list(self.read_mmap[idx:idx+size])
        # ARM uses little-endian.
        # Need to flip bytes around since we're reading individual chars.
        flip_endianness(insn)
        return insn

    def write(self, i, insn):
        """
        Write a 32-bit instruction back to vmlinux.instr.

        insn can be a list of 4 chars or an 32-bit integer in big-endian format.
        Converts back to little-endian (ARM's binary format) before writing.
        """
        size = 4
        idx = self._insn_idx(i)
        insn = list(byte_string(insn))
        flip_endianness(insn)
        self.write_mmap[idx:idx+size] = byte_string(insn)

    def close(self):
        self.flush()

        self.write_mmap.close()
        self.write_f.close()
        self.read_mmap.close()
        self.read_f.close()

        self.write_mmap = None
        self.write_f = None
        self.read_mmap = None
        self.read_f = None

    def is_conf_set(self, var):
        if self.conf is None:
            return None
        return self.conf.get(var) == 'y'

    def get_conf(self, var):
        if self.conf is None:
            return None
        return self.conf.get(var)

    def flush(self):
        self.write_mmap.flush()
        self.write_f.flush()

    def line(self, i):
        """
        Return the i-th (0-based) line of output from "aarch64-linux-android-objdump -d vmlinux".

        (no lines are filtered).
        """
        return self.lines[i][0]
    def _section_idx(self, i):
        return self.lines[i][1]
    def virt_addr(self, i):
        return self.lines[i][2]

    def section(self, section_name):
        """
        >>> self.section('.text')
        {'address': 18446743798847711608L,
         'align': 1,
         'lma': 18446743798847711608L,
         'name': '.text',
         'number': 23,
         'offset': 15608184,
         'size': '0017aae8',
         'type': 'ALLOC'},
        """
        return self.sections['sections'][self.sections['section_idx'][section_name]]

    def _section(self, i):
        return self.sections['sections'][self._section_idx(i)]

    def is_func(self, i):
        return bool(self.get_func(i))

    def get_func(self, i):
        return re.search(common.fun_rec, self.line(i))

    def is_insn(self, i):
        """
        Returns True if self.line(i) is an instruction
        (i.e. not a function label line, blank line, etc.)
        """
        return not self.is_func(i) and self.virt_addr(i) is not None

    FUNC_OFFSET_RE = re.compile(r'^(?P<virt_addr>{hex_re})'.format(hex_re=hex_re))
    def get_func_idx(self, func, i=None):
        i_set = self.func_idx[func]
        if len(i_set) != 1 and i is None:
            raise RuntimeError("{func} occurs multiple times in vmlinux, specify which line from objdump you want ({i_set})".format(**locals()))
        elif i is None:
            i = iter(i_set).next()
        else:
            assert i in i_set
        return i
    def get_func_end_idx(self, func, start_i=None):
        i = self.get_func_idx(func, start_i)
        while i < len(self.lines) and ( self.is_func(i) or self.is_insn(i) ):
            i += 1
        return i - 1
    def func_offset(self, func, i=None):
        i = self.get_func_idx(func, i)
        m = re.search(Objdump.FUNC_OFFSET_RE, self.line(i))
        return _int(m.group('virt_addr'))

    PARSE_INSN_RE = re.compile((
        r'(?P<virt_addr>{hex_re}):\s+'
        r'(?P<hex_insn>{hex_re})\s+'
        r'(?P<type>[^\s]+)\s*'
        ).format(hex_re=hex_re))
    def parse_insn(self, i):
        """
        Parse the i-th line of objdump output into a python dict.

        e.g.
        >>> self.line(...)
        [2802364][97 DB 48 D0] :: ffffffc0014ee5b4:     97db48d0        bl      ffffffc000bc08f4 <rmnet_vnd_exit>

        >>> self.parse_insn(2802364)
        {'args': {'offset': -9624768},             # 'args' field varies based on instruction type.
         'binary': ['\x97', '\xdb', 'H', '\xd0'],  # The remaining fields are always present.
         'hex_insn': '97db48d0',
         'type': 'bl',
         'virt_addr': 18446743798853592500L}
        """
        line = self.line(i)
        m = re.search(Objdump.PARSE_INSN_RE, line)
        insn = m.groupdict()
        insn['virt_addr'] = _int(insn['virt_addr'])
        insn['binary'] = self.read(i)

        insn['args'] = {}
        if insn['type'] == 'bl':
            # imm26 (bits 0..25)
            insn['args']['offset'] = from_twos_compl((hexint(insn['binary']) & BL_OFFSET_MASK) << 2, nbits=26 + 2)
        elif insn['type'] in set(['blr', 'ret']):
            arg = {
            'blr':'dst_reg',
            'ret':'target_reg',
            }[insn['type']]
            insn['args'][arg] = (hexint(insn['binary']) & BLR_AND_RET_RN_MASK) >> 5
        elif insn['type'] == 'stp':
            insn['args']['reg1']     = mask_shift(insn , STP_RT_MASK              , 0)
            insn['args']['base_reg'] = mask_shift(insn , STP_RN_MASK              , 5)
            insn['args']['reg2']     = mask_shift(insn , STP_RT2_MASK             , 10)
            insn['args']['opc']      = mask_shift(insn , STP_OPC_MASK             , 30)
            insn['args']['mode']     = mask_shift(insn , STP_ADDRESSING_MODE_MASK , 22)
            lsl_bits = stp_lsl_bits(insn)
            insn['args']['imm'] = from_twos_compl(
                    ((hexint(insn['binary']) & STP_IMM7_MASK) >> 15) << lsl_bits,
                    nbits=7 + lsl_bits)
        elif mask_shift(insn, ADDIMM_OPCODE_MASK, 24) == ADDIM_OPCODE_BITS \
                and insn['type'] in set(['add', 'mov']):
            insn['type'] = 'add'
            insn['args']['sf'] = mask_shift(insn, ADDIMM_SF_MASK, 31)
            insn['args']['shift'] = mask_shift(insn, ADDIMM_SHIFT_MASK, 22)
            insn['args']['imm'] = mask_shift(insn, ADDIMM_IMM_MASK, 10)
            insn['args']['src_reg'] = mask_shift(insn, ADDIMM_RN_MASK, 5)
            insn['args']['dst_reg'] = mask_shift(insn, ADDIMM_RD_MASK, 0)
            insn['args']['opcode_bits'] = mask_shift(insn, ADDIMM_OPCODE_MASK, 24)
        elif insn['type'] == 'adrp':
            immlo = mask_shift(insn, ADRP_IMMLO_MASK, ADRP_IMMLO_ZBITS)
            immhi = mask_shift(insn, ADRP_IMMHI_MASK, ADRP_IMMHI_ZBITS)
            insn['args']['dst_reg'] = mask_shift(insn, ADRP_RD_MASK, ADRP_RD_ZBITS)
            insn['args']['imm'] = from_twos_compl((immhi << (2 + 12)) | (immlo << 12), nbits=2 + 19 + 12)
        elif insn['type'] == 'ldp':
            insn['args']['reg1']     = mask_shift(insn , STP_RT_MASK              , 0)
            insn['args']['base_reg'] = mask_shift(insn , STP_RN_MASK              , 5)
            insn['args']['reg2']     = mask_shift(insn , STP_RT2_MASK             , 10)
        elif mask_shift(insn, ADDIMM_OPCODE_MASK, 24) == ADDIM_OPCODE_BITS \
                and insn['type'] in set(['add', 'mov']):
                insn['type'] = 'add'
                insn['args']['sf'] = mask_shift(insn, ADDIMM_SF_MASK, 31)
                insn['args']['shift'] = mask_shift(insn, ADDIMM_SHIFT_MASK, 22)
                insn['args']['imm'] = mask_shift(insn, ADDIMM_IMM_MASK, 10)
                insn['args']['src_reg'] = mask_shift(insn, ADDIMM_RN_MASK, 5)
                insn['args']['dst_reg'] = mask_shift(insn, ADDIMM_RD_MASK, 0)
                insn['args']['opcode_bits'] = mask_shift(insn, ADDIMM_OPCODE_MASK, 24)
        else:
            insn['args']['raw'] = line[m.end():]
        return insn

    def encode_insn(self, insn):
        """
        Given a python dict representation of an instruction (see parse_insn), write its
        binary to vmlinux.instr.

        TODO:
        stp x29, xzr, [sp,#<frame>]
        str x30, [sp,#<frame - 8>]
        add x29, sp, offset
        """
        if insn['type'] == 'eor':
            upper_11_bits =0b11001010000
            return (upper_11_bits << 21) | (insn['args']['reg3'] << 16) | (0b000000<<10) | \
                    (insn['args']['reg2'] << 5) |(insn['args']['reg1'])
        elif insn['type'] == 'eor_imm':
            upper_10_bits =0b1101001001
            return (upper_10_bits << 22) | (0b010001<<16) | (0b000000<<10) | \
                    (insn['args']['reg2'] << 5) |(insn['args']['reg1'])
        elif insn['type'] == 'ldp':
            return (0b1010100111 << 22) | (insn['args']['reg2'] << 10) | \
                    (insn['args']['base_reg'] << 5) | (insn['args']['reg1'])
        elif insn['type'] in ['bl', 'b']:
            # BL: 1 0 0 1 0 1 [ imm26 ]
            #  B: 0 0 0 1 0 1 [ imm26 ]
            upper_6_bits = {
                'bl':0b100101,
                 'b':0b000101,
            }[insn['type']]
            assert 128*1024*1024 >= insn['args']['offset'] >= -128*1024*1024
            return ( upper_6_bits << 26 ) | (to_twos_compl(insn['args']['offset'], nbits=26 + 2) >> 2)
        elif insn['type'] in ['blr', 'ret']:
            #      1 1 0 1 0 1 1 0 0  [ op ] 1 1 1 1 1 0 0 0 0 0 0 [   Rn   ] 0 0 0 0 0
            # BLR:                     0  1
            # RET:                     1  0
            op = {
                'blr':0b01,
                'ret':0b10,
            }[insn['type']]
            assert 0 <= insn['args']['dst_reg'] <= 2**5 - 1
            return (0b110101100 << 25) | \
                   (op << 21) | \
                   (0b11111000000 << 10) | \
                   (insn['args']['dst_reg'] << 5)
        elif insn['type'] == 'ret':
            # 1 1 0 1 0 1 1 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0 [  Rn   ] 0 0 0 0 0
            assert 0 <= insn['args']['dst_reg'] <= 2**5 - 1
            return (0b1101011000111111000000 << 9) | (insn['args']['dst_reg'] << 4)
        elif insn['type'] == 'stp':
            assert insn['args']['opc'] == OPC_64
            return (insn['args']['opc'] << 30) | \
                   (insn['args']['mode'] << 22) | \
                   (to_twos_compl(insn['args']['imm'] >> stp_lsl_bits(insn), nbits=7) << 15) | \
                   (insn['args']['reg2'] << 10) | \
                   (insn['args']['base_reg'] << 5) | \
                   insn['args']['reg1']
        elif insn['type'] == 'str' and \
                insn['args']['mode'] == STR_IMM_OFFSET and \
                insn['args']['sign'] == STR_SIGN_UNSIGNED:
            assert insn['args']['imm'] >= 0
            return (insn['args']['size'] << 30) | \
                   (0b111001 << 24) | \
                   (insn['args']['opc'] << 22) | \
                   (to_twos_compl(insn['args']['imm'] >> str_lsl_bits(insn), nbits=12) << 10) | \
                   (insn['args']['base_reg'] << 5) | \
                   (insn['args']['reg1'] << 0)
        elif insn['type'] == 'add' and insn['args']['opcode_bits'] == ADDIM_OPCODE_BITS:
            assert insn['args']['sf'] == ADDIMM_SF_BIT_64
            return \
                (insn['args']['sf'] << 31) | \
                (insn['args']['shift'] << 22) | \
                (insn['args']['imm'] << 10) | \
                (insn['args']['src_reg'] << 5) | \
                (insn['args']['dst_reg'] << 0) | \
                (insn['args']['opcode_bits'] << 24)
        elif insn['type'] in ['mov', 'movk', 'movn']:
            opc = {
                'mov':0b10,
                'movk':0b11,
                'movn':0b00,
            }[insn['type']]
            hw = {
                0:0b00,
                16:0b01,
                32:0b11,
            }[insn['args']['shift']]
            sf = 0b1 # 64-bit registers
            return (sf << 31) | \
                   (opc << 29) | \
                   (0b100101 << 23) | \
                   (hw << 21) | \
                   (insn['args']['imm16'] << 5) | \
                   (insn['args']['dst_reg'])
        elif insn['type'] == 'nop':
            return 0xd503201f
        raise NotImplementedError

    def funcs(self):
        """
        [ ("func_1", 0), ("func_2", 1), ... ]
        """
        def __funcs():
            for func, i_set in self.func_idx.iteritems():
                for i in i_set:
                    yield func, i
        funcs = list(__funcs())
        funcs.sort(key=lambda func_i: func_i[1])
        return funcs

    def _idx_to_func(self, i):
        if self._funcs is None:
            self._funcs = self.funcs()
        lo = 0
        hi = len(self._funcs) - 1
        mi = None
        def func(i):
            return self._funcs[i][0]
        def idx(i):
            return self._funcs[i][1]
        while lo <= hi:
            mi = (hi + lo)/2
            if i < idx(mi):
                hi = mi-1
            elif i > idx(mi):
                lo = mi+1
            else:
                return i
        assert lo == hi + 1
        return idx(hi)

    def each_insn(self,
            # Instrument a single function.
            func=None,
            # Instrument a range of functions.
            start_func=None, end_func=None,
            # Start index into objdump of function (NEED this to disambiguate duplicate symbols)
            start_i=None, end_i=None,
            # If skip_func(func_name), skip it.
            skip_func=None,
            just_insns=False,
            # Don't parse instruction, just give raw objdump line.
            raw_line=False,
            # Number of past instruction lines to yield with the current one.
            num_last_insns=None,
            debug=False):
        """
        Iterate over instructions (i.e. line indices and their parsed python dicts).

        Default is entire file, but can be limited to just a function.
        """
        if func:
            start_func = func
            end_func = func

        i = 0
        if start_func is not None:
            i = self.get_func_idx(start_func, start_i)
        elif start_i is not None:
            i = start_i
            start_func = self._idx_to_func(i)
        else:
            # The first function
            start_func, i = self.funcs()[0]
        func_i = i
        curfunc = start_func

        end = len(self.lines) - 1
        if end_func is not None:
            end = self.get_func_end_idx(end_func, end_i)
        assert not( end_func is None and end_i is not None )

        def should_skip_func(func):
            return skip_func is not None and skip_func(func)
        assert start_func is not None

        last_insns = None
        num_before_start = 0
        if num_last_insns is not None:
            last_insns = [(None, None)] * num_last_insns
            assert len(last_insns) == num_last_insns
            # Walk backwards from the start until we see num_last_insns instructions.
            # j is the index of the instruction.
            # That new starting point (i) will be that many instructions back.
            n = num_last_insns
            j = i - 1
            while n > 0 and j > 0:
                if self.is_insn(j):
                    n -= 1
                j -= 1
            new_i = j + 1
            num_before_start = i - new_i + 1
            i = new_i
        def shift_insns_left(last_insns, i, to_yield):
            last_insns.pop(0)
            last_insns.append((i, to_yield))

        do_skip_func = should_skip_func(start_func)
        def _tup(i, curfunc, func_i, to_yield):
            if just_insns:
                return i, to_yield
            else:
                return curfunc, func_i, i, to_yield

        def _parse(to_yield, i):
            if to_yield is None:
                return self.line(i) if raw_line else self.parse_insn(i)
            return to_yield

        for i in xrange(i, min(end, len(self.lines) - 1) + 1):

            to_yield = None

            if num_last_insns is not None and num_before_start != 0:
                if self.is_insn(i):
                    to_yield = _parse(to_yield, i)
                    shift_insns_left(last_insns, i, to_yield)
                num_before_start -= 1
                continue

            if self.is_insn(i):
                to_yield = _parse(to_yield, i)
                if not do_skip_func:
                    t = _tup(i, curfunc, func_i, to_yield)
                    if num_last_insns is not None:
                        yield t + (last_insns,)
                    else:
                        yield t
                if num_last_insns is not None:
                    shift_insns_left(last_insns, i, to_yield)
            else:
                m = self.get_func(i)
                if m:
                    curfunc = m.group('func_name')
                    func_i = i
                    do_skip_func = should_skip_func(curfunc)

    def each_insn_parallel(self, each_insn, threads=1, **kwargs):
        """
        each_insn(start_func=None, end_func=None, start_i=None, end_i=None)
        """
        # Spawn a bunch of threads to instrument in parallel.
        procs = []
        i = 0
        funcs = self.funcs()
        chunk = int(math.ceil(len(funcs)/float(threads)))
        for n in xrange(threads):
            start_func_idx = i
            end_func_idx = min(i+chunk-1, len(funcs)-1)
            start_i = funcs[start_func_idx][1]
            end_i = funcs[end_func_idx][1]
            kwargs.update({
                'start_func':funcs[start_func_idx][0],
                'end_func':funcs[end_func_idx][0],
                'start_i':start_i,
                'end_i':end_i,
                'tid':n,
                })
            if threads == 1:
                each_insn(**kwargs)
                return
            proc = multiprocessing.Process(target=each_insn, kwargs=kwargs)

            i = end_func_idx + 1
            proc.start()
            procs.append(proc)
            if i >= len(funcs):
                break
        for proc in procs:
            proc.join()

def each_procline(proc):
    """
    Iterate over the stdout lines of subprocess.Popen(...).
    """
    while True:
        line = proc.stdout.readline()
        if line != '':
            yield line.rstrip()
        else:
            break

"""
Replace an instruction with a new one.

These functions modify the python dict's returned by Objdump.each_insn.
Instructions can then be written to vmlinux.instr using Objdump.write(i, Objdump.encode_insn(insn)).
"""
def bl_insn(insn, offset):
    return _jmp_offset_insn(insn, 'bl', offset)
def _jmp_offset_insn(insn, typ, offset):
    insn['type'] = typ
    insn['args'] = { 'offset': offset }
    return insn

def eor_insn(insn, reg1, reg2, reg3):
    insn['type'] = 'eor'
    insn['args'] = {
        'reg1':reg1,
        'reg2':reg2,
        'reg3':reg3,
    }
    return insn

def eor_imm_insn(insn, reg1, reg2):
    insn['type'] = 'eor_imm'
    insn['args'] = {
        'reg1':reg1,
        'reg2':reg2,
    }
    return insn

def stp_insn(insn, reg1, reg2, base_reg, imm, mode):
    insn['type'] = 'stp'
    insn['args'] = {
        'reg1':reg1,
        'base_reg':base_reg,
        'reg2':reg2,
        'opc':OPC_64,
        'mode':mode,
        'imm':imm,
    }
    return insn

NM_RE = re.compile(r'(?P<addr>.{16}) (?P<symbol_type>.) (?P<symbol>.*)')
NE_TYPE = 0
NE_ADDR = 1
NE_SIZE = 2
def parse_nm(vmlinux, symbols=None):
    """
    MAJOR TODO:
    Must handle functions (symbols) that occur more than once!
    e.g.
    add_dirent_to_buf is a static function defined in both:
    - fs/ext3/namei.c
    - fs/ext4/namei.c

    ffffffc0000935b0 T cpu_resume_mmu
    ffffffc0000935c0 t cpu_resume_after_mmu
    ...
    ffffffc0000935f0 D cpu_resume
    ...
    ffffffc000093680 T __cpu_suspend_save
    ...
    ffffffc000c60000 B _end
                     U el
                     U lr

    {
      'cpu_resume':('D', 'ffffffc0000935f0', 36)
    }
    """
    proc = subprocess.Popen(["{NM} {vmlinux} | sort".format(NM=NM, vmlinux=vmlinux)], shell=True, stdout=subprocess.PIPE)
    f = each_procline(proc)
    nm = {}
    last_symbol = None
    last_name = None
    for line in f:
        m = re.search(NM_RE, line)
        if m:
            if last_symbol is not None and ( symbols is None or last_name in symbols ):
                last_symbol[NE_SIZE] = ( _int(m.group('addr')) - _int(last_symbol[NE_ADDR]) ) / BYTES_PER_INSN \
                            if \
                                re.match(hex_re, last_symbol[NE_ADDR]) and \
                                re.match(hex_re, m.group('addr')) \
                            else None
            last_symbol = [m.group('symbol_type'), m.group('addr'), None]
            last_name = m.group('symbol')
            if symbols is None or m.group('symbol') in symbols:
                nm[m.group('symbol')] = last_symbol
    return nm

def addr_to_section(hexaddr, sections):
    addr = _int(hexaddr)
    for section in sections:
        if section['address'] <= addr < section['address'] + section['size']:
            return section

def parse_sections(vmlinux):
    """
    [Nr] Name              Type             Address           Offset
         Size              EntSize          Flags  Link  Info  Align
    [ 0]                   NULL             0000000000000000  00000000
         0000000000000000  0000000000000000           0     0     0
    [ 1] .head.text        PROGBITS         ffffffc000205000  00005000
         0000000000000500  0000000000000000  AX       0     0     64
    {
      'name': '.head.text',
      'size': 0,
      'type': PROGBITS,
      ...
    }
    """
    proc = subprocess.Popen([OBJDUMP, '--section-headers', vmlinux], stdout=subprocess.PIPE)
    f = each_procline(proc)
    d = {
        'sections': [],
        'section_idx': {},
    }
    it = iter(f)
    section_idx = 0
    while True:
        try:
            line = it.next()
        except StopIteration:
            break

        m = re.search(r'^Sections:', line)
        if m:
            # first section
            it.next()
            continue

        m = re.search((
            # [Nr] Name              Type             Address           Offset
            r'^\s*(?P<number>\d+)'
            r'\s+(?P<name>[^\s]*)'
            r'\s+(?P<size>{hex_re})'
            r'\s+(?P<address>{hex_re})'
            r'\s+(?P<lma>{hex_re})'
            r'\s+(?P<offset>{hex_re})'
            r'\s+(?P<align>[^\s]+)'
            ).format(hex_re=hex_re), line)
        if m:
            section = {}

            d['section_idx'][m.group('name')] = int(m.group('number'))

            def parse_power(x):
                m = re.match(r'(?P<base>\d+)\*\*(?P<exponent>\d+)', x)
                return int(m.group('base'))**int(m.group('exponent'))
            section.update(coerce(m.groupdict(), [
                [_int, ['size', 'address', 'offset', 'lma']],
                [int, ['number']],
                [parse_power, ['align']]]))

            line = it.next()
            # CONTENTS, ALLOC, LOAD, READONLY, CODE
            m = re.search((
            r'\s+(?P<type>.*)'
            ).format(hex_re=hex_re), line)
            section.update(m.groupdict())

            d['sections'].append(section)

    return d

def coerce(dic, funcs, default=lambda x: x):
    field_to_func = {}
    for row in funcs:
        f, fields = row
        for field in fields:
            field_to_func[field] = f

    fields = dic.keys()
    for field in fields:
        if field not in field_to_func:
            continue
        dic[field] = field_to_func[field](dic[field])

    return dic

def _int(hex_string):
    """
    Convert a string of hex characters into an integer

    >>> _int("ffffffc000206028")
    18446743798833766440L
    """
    return int(hex_string, 16)
def _hex(integer):
    return re.sub('^0x', '', hex(integer)).rstrip('L')

def main():
    parser = argparse.ArgumentParser("Instrument vmlinux to protect against kernel code reuse attacks")
    parser.add_argument("--vmlinux", required=True,
            help="vmlinux file to run objdump on")
    parser.add_argument("--config",
            help="kernel .config file; default = .config in location of vmlinux if it exists")
    parser.add_argument("--threads", type=int, default=DEFAULT_THREADS,
            help="Number of threads to instrument with (default = # of CPUs on machine)")
    parser.add_argument("--inplace", action='store_true',
            help="instrument the vmlinux file inplace")

    args = parser.parse_args()

    if args.vmlinux is None:
        parser.error("Need top directory of vmlinux for --vmlinux")

    if args.config is None:
        parser.error("Need top directory of .config for --config")

    if args.threads is None:
        parser.error("Please use --threads or set DEFAULT_THREADS")

    if args.inplace is None:
        parser.error("Please use --inplace")

    if not os.path.exists(args.vmlinux):
        parser.error("--vmlinux ({vmlinux}) doesn't exist".format(vmlinux=args.vmlinux))

    def _load_objdump():
        return contextlib.closing(load_and_cache_objdump(args.vmlinux, config_file=args.config, inplace=args.inplace))

    # instrument and validate
    with _load_objdump() as objdump:
        instrument(objdump, func=None, skip=common.skip, skip_stp=common.skip_stp,
                skip_asm=common.skip_asm, skip_blr=common.skip_blr, keep_magic=common.keep_magic, threads=args.threads)
        #objdump.save_instr_copy()
        return

def each_line(fname):
    with open(fname) as f:
        for line in f:
            line = line.rstrip()
            yield line

def parse_config(config_file):
    """
    Parse kernel .config
    """
    conf = {}
    for line in each_line(config_file):
        m = re.search(r'^\s*(?P<var>[A-Z0-9_]+)=(?P<value>[^\s#]+)', line)
        if m:
            conf[m.group('var')] = m.group('value')
    return conf

def load_and_cache_objdump(vmlinux, *objdump_args, **objdump_kwargs):
    """
    Parse vmlinux into an Objdump.
    """
    objdump = Objdump(vmlinux, *objdump_args, **objdump_kwargs)
    objdump.parse()
    objdump.open()

    return objdump

def flip_endianness(word):
    assert len(word) == 4
    def swap(i, j):
        tmp = word[i]
        word[i] = word[j]
        word[j] = tmp
    swap(0, 3)
    swap(1, 2)

def from_twos_compl(x, nbits):
    """
    Convert nbit two's compliment into native decimal.
    """
    # Truely <= nbits long?
    assert x == x & ((2**nbits) - 1)
    if x & (1 << (nbits - 1)):
        # sign bit is set; it's negative
        flip = -( (x ^ (2**nbits) - 1) + 1 )
        # twiddle = ~x + 1
        return flip
    return x

def to_twos_compl(x, nbits):
    """
    Convert native decimal into nbit two's complement
    """
    if x < 0:
        flip = (( -x ) - 1) ^ ((2**nbits) - 1)
        assert flip == flip & ((2**nbits) - 1)
        return flip
    return x

def byte_string(xs):
    if type(xs) == list:
        return ''.join(xs)
    elif type(xs) in [int, long]:
        return ''.join([chr((xs >> 8*i) & 0xff) for i in xrange(3, -1, 0-1)])
    return xs
def hexint(b):
    return int(binascii.hexlify(byte_string(b)), 16)
def mask_shift(insn, mask, shift):
    return (hexint(insn['binary']) & mask) >> shift
def mask(insn, mask):
    return hexint(insn['binary']) & mask

def my_dirname(fname):
    """
    If file is in current directory, return '.'.
    """
    dirname = os.path.dirname(fname)
    if dirname == '':
        dirname = '.'
    return dirname

def are_nop_insns(insns):
    return all(ins is not None and ins['type'] == 'nop' for ins in insns)

def stp_lsl_bits(insn):
    return (2 + (insn['args']['opc'] >> 1))

def str_lsl_bits(insn):
    """
    ARMv8 Manual:
    integer scale = UInt(size);
    bits(64) offset = LSL(ZeroExtend(imm12, 64), scale);
    """
    return insn['args']['size']

if common.run_from_ipython():
    """
    Iterative development is done using ipython REPL.
    This code only runs when importing this module from ipython.
    Instrumentation will be created in a copy of that file (with a .instr suffix).

    ==== How to test ====

    >>> ... # means to type this at the ipython terminal prompt

    # To reload your code after making changes, do:
    change the DEFAULT_THREADS to 1 before debugging
    >>> import instrument; dreload(instrument)

    # To instrument vmlinux, do:
    >>> instrument._instrument()
    """
    # Define some useful stuff for debugging via ipython.

    # Set this to a vmlinux file we want to copy then instrument, not correct for QC
    sample_vmlinux_file = os.path.expandvars("../../vmlinux")
    sample_config_file = os.path.expandvars("../../.config")

    #import pdb; pdb.set_trace()
    o = load_and_cache_objdump(sample_vmlinux_file, config_file=sample_config_file)

    print "in function common.run_from_ipython()"

    def _instrument(func=None, skip=common.skip, validate=True, threads=DEFAULT_THREADS):
        instrument(o, func=func, skip=common.skip, skip_stp=common.skip_stp, skip_asm=common.skip_asm, threads=threads)
        o.flush()

if __name__ == '__main__':
    main()
