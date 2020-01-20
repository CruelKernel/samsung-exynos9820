#!/usr/bin/env python
# Code for validating instrumention (i.e. for detecting bugs in instrument.py).
import re
import multiprocessing
import textwrap
import subprocess
import mmap

import instrument

import common
from common import pr, log

register_re = r'(?:(x|w)\d+|xzr|sp)'
hex_char_re = r'(?:[a-f0-9])'

def validate_instrumentation(objdump_uninstr, skip, skip_stp, skip_asm, skip_save_lr_to_stack, skip_br, threads=1):
    """
    Make sure that we instrumented vmlinux properly by checking some properties from its objdump.

    Properties to check for:
    - make sure there aren't any uninstrumented instructions
      - i.e. a bl instruction that doesn't go through the springboard
    - make sure there aren't any assembly routines that do things with LR that would keep us from re-encrypting it properly
      - e.g. storing x30 in a callee saved register (instead of placing it on the stack and adjusting x29)

        el1_preempt:
            mov	x24, x30
            ...
            ret	x24
    - make sure there aren't any uninstrumented function prologues
      i.e.
        <assembled_c_function>:
          (not a nop)
          stp	x29, x30, [sp,#-<frame>]!
          (insns)
          mov	x29, sp

        <assembled_c_function>:
          nop
          stp	x29, x30, [sp,#-<frame>]!
          (insns)
          mov	x29, sp

        <assembled_c_function>:
          nop
          stp	x29, x30, [sp,#<offset>]
          add	x29, sp, #<offset>

        <assembled_c_function>:
          (not a nop)
          stp	x29, x30, [sp,#<offset>]
          add	x29, sp, #<offset>
    """

    lock = multiprocessing.Lock()
    success = multiprocessing.Value('i', True)

    def insn_text(line):
        """
        >>> insn_text("ffffffc000080148:	d503201f 	nop")
        "nop"
        """
        m = re.search(r'^{hex_char_re}{{16}}:\s+{hex_char_re}{{8}}\s+(.*)'.format(
            hex_char_re=hex_char_re), line)
        if m:
            return m.group(1)
        return ''

    #
    # Error reporting functions.
    #
    def _msg(list_of_func_lines, msg, is_failure):
        with lock:
            if len(list_of_func_lines) > 0:
                log(textwrap.dedent(msg))
                for func_lines in list_of_func_lines:
                    log()
                    for line in func_lines:
                        log(line.rstrip('\n'))
                success.value = False
    def errmsg(list_of_func_lines, msg):
        _msg(list_of_func_lines, msg, True)
    def warmsg(list_of_func_lines, msg):
        _msg(list_of_func_lines, msg, False)
    def err(list_of_args, msg, error):
        with lock:
            if len(list_of_args) > 0:
                log(textwrap.dedent(msg))
                for args in list_of_args:
                    log()
                    log(error(*args).rstrip('\n'))
                success.value = False

    asm_functions = instrument.parse_all_asm_functions(objdump_uninstr.kernel_src)
    c_functions = objdump_uninstr.c_functions

    #
    # Validation functions. Each one runs in its own thread.
    #

    def validate_bin():
        # Files must differ.
        # subprocess.check_call('! diff -q {vmlinux_uninstr} {vmlinux_instr} > /dev/null'.format(
        #     vmlinux_uninstr=objdump_uninstr.vmlinux_old, vmlinux_instr=objdump_uninstr.instr), shell=True)
        cmd = 'cmp -l {vmlinux_uninstr} {vmlinux_instr}'.format(
            vmlinux_uninstr=objdump_uninstr.vmlinux_old, vmlinux_instr=objdump_uninstr.instr) + \
                " | gawk '{printf \"%08X %02X %02X\\n\", $1, strtonum(0$2), strtonum(0$3)}'"
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
        f = instrument.each_procline(proc)
        to_int = lambda x: int(x, 16)
        bin_errors = []
        for line in f:
            byte_offset, byte1, byte2 = map(to_int, re.split(r'\s+', line))
            byte_offset -= 1
            section = instrument.offset_to_section(byte_offset, objdump_uninstr.sections['sections'])
            if section is None:
                name = 'None'
                bin_errors.append((byte_offset, None, None))
            else:
                addr = section['address'] + byte_offset - section['offset']
                if 'CODE' not in section['type'] or section['name'] == '.vmm':
                    bin_errors.append((byte_offset, addr, section['name']))
        def _to_str(byte_offset, addr, section):
            if section is None:
                return "byte offset 0x{byte_offset}".format(
                    byte_offset=instrument._hex(byte_offset))
            return "0x{addr} (byte offset 0x{byte_offset}) in section {section}".format(
                addr=instrument._hex(addr), byte_offset=instrument._hex(byte_offset), section=section)
        err(bin_errors, """
        Saw changes in binary sections of instrumented vmlinux that should not be there!
        Changes should only be in the code.
        """, error=_to_str)

    def validate_instr():
        """
        Validations to perform on the instrumented vmlinux.
        """

        objdump_instr = instrument.load_and_cache_objdump(objdump_uninstr.instr,
                kernel_src=objdump_uninstr.kernel_src, config_file=objdump_uninstr.config_file, make_copy=False, just_lines=True)

        uninstrumented_br = []
        uninstrumented_blr = []

        def err_uninstr_branch(uninstr_lines):
            with lock:
                if len(uninstr_lines) > 0:
                    log()
                    log(textwrap.dedent("""
                    ERROR: instrumentation does not look right (instrument.py has a bug).
                    These lines in objdump of vmlinux_instr aren't instrumented correcly:
                    """))
                    n = min(5, len(uninstr_lines))
                    for line in uninstr_lines[0:n]:
                        log(line)
                    if n < len(uninstr_lines):
                        log("...")
                    success.value = False

        def is_uninstr_blr_branch(func, branch_pattern, uninstr_lines):
            if not func.startswith('jopp_springboard_') and (
                    re.search(branch_pattern, line) and not re.search(r'<jopp_springboard_\w+>', line)
                    ):
                uninstr_lines.append(line)
                return True
        uninstrumented_prologue_errors = []
        prologue_errors = []
        nargs_errors = []
        #import pdb; pdb.set_trace()
        check_prologue = objdump_instr.is_conf_set('CONFIG_RKP_CFP_ROPP')
        for func, lines, last_insns in objdump_instr.each_func_lines(num_last_insns=2):
            if instrument.skip_func(func, skip, skip_asm):
                continue
            prologue_error = False
            # TODO: This check incorrectly goes off for cases where objdump skips showing 0 .word's.
            # e.g.
            # ffffffc000c25d74:	d503201f 	nop
            # 	...
            ## ffffffc000c25d84:	b3ea3bad 	.inst	0xb3ea3bad ; undefined
            ##
            # ffffffc003c25d88 <vcs_init>:
            # ffffffc000c25d88:	a9bd7ffd 	stp	x29, xzr, [sp,#-48]!
            # ffffffc000c25d8c:	910003fd 	mov	x29, sp
            # ...
            #
            for i, line in enumerate(lines):
                #for checking BR
                if re.search(r'\s+br\t', line) and (func not in skip_br):
                    uninstrumented_br.append(line)

                # for checking BLR
                if is_uninstr_blr_branch(func, r'\s+blr\t', uninstrumented_blr):
                    continue
                # Detect uninstrumented prologues:
                # nop           <--- should be eor RRX, x30, RRK
                # stp x29, 30
                if re.search(r'^nop', insn_text(line)) and i + 1 < len(lines) and re.search(r'stp\tx29, x30, .*sp', lines[i+1]):
                    uninstrumented_prologue_errors.append(lines)
                    continue
                if check_prologue:
                    m = re.search(r'stp\tx29, x30, .*sp', line)
                    if m and func not in skip_stp:
                        # We are in error if "stp x29, x30, [sp ..." exists in this function.
                        # (hopefully this doesn't raise false alarms in any assembly functions)
                        prologue_error = True
                        continue
            if prologue_error:
                prologue_errors.append(lines)

        err_uninstr_branch(uninstrumented_br)
        err_uninstr_branch(uninstrumented_blr)
        errmsg(prologue_errors, """
        Saw an assembly routine(s) that looks like it is saving x29 and x30 on the stack, but
        has not been instrumented to save x29, xzr FIRST.
        i.e.
        Saw:
          stp	x29, x30, [sp,#-<frame>]!
          (insns)
          mov	x29, sp                     <--- might get preempted just before doing this
                                                 (won't reencrypt x30!)
        Expected:
          stp x29, xzr, [sp,#-<frame>]!
          mov x29, sp                       <--- it's ok if we get preempted
          (insns)                                (x30 not stored yet)
          str x30, [sp,#<+ 8>]
        """)
        errmsg(nargs_errors, """
        Saw a dissassembled routine that doesn't have the the "number of function
        arugments" and the "function entry point magic number" annotated above it.
        """)
        errmsg(uninstrumented_prologue_errors, """
        Saw a function that doesn't have an instrumented C prologue.
        In particular, we saw:
        <func>:
          nop
          stp x29, x30, ...
          ...

        But we expected to see:
        <func>:
          eor RRX, x30, RRK
          stp x29, x30, ...
          ...
        """)

    def validate_uninstr_binary():
        """
        Validations to perform on the uninstrumented vmlinux binary words.
        """
        if objdump_uninstr.JOPP_CHECK_MAGIC_NUMBER_ON_BLR:
            magic_errors = []
            def each_word(section):
                read_f = open(objdump_uninstr.vmlinux_old, 'rb')
                read_f.seek(0)
                read_mmap = mmap.mmap(read_f.fileno(), 0, access=mmap.ACCESS_READ)
                try:
                    i = section['offset']
                    while i + 4 < section['size']:
                        word = read_mmap[i:i+4]
                        yield i, word
                        i += 4
                finally:
                    read_mmap.close()
                    read_f.close()
            for section in objdump_uninstr.sections['sections']:
                if 'CODE' in section['type']:
                    # Make sure JOPP_FUNCTION_ENTRY_POINT_MAGIC_NUMBER
                    # doesn't appear in a word of an uninstrumented vmlinux.
                    for i, word in each_word(section):
                        if word == objdump_uninstr.JOPP_FUNCTION_ENTRY_POINT_MAGIC_NUMBER:
                            magic_errors.append([i, section])
            err(magic_errors, """
            The magic number chosen to place at the start of every function already
            appears in the uninstrumented vmlinux.  Find a new magic number!
            (JOPP_FUNCTION_ENTRY_POINT_MAGIC_NUMBER = {JOPP_FUNCTION_ENTRY_POINT_MAGIC_NUMBER})
            """,
            error=lambda i, section: "0x{addr} in section {section}".format(
                addr=instrument._hex(i + section['address']), section=section['name']))
    def validate_uninstr_lines():
        """
        Validations to perform on the uninstrumented vmlinux objdump lines.
        """
        if objdump_uninstr.JOPP_FUNCTION_NOP_SPACERS:
            # Assume that the key might change and require return-address reencryption.  This
            # means we need to have all copies of x30 either in x30 itself, or saved in memory
            # and pointed to by a frame pointer.
            #
            # In particular, we can't allow return-addresses being saved in callee registers
            # as is done in some low-level assembly routines, since when the key changes these
            # registers will become invalid and not be re-encrypted.
            #
            # Look for and warn about:
            #
            # mov <rd>, x30
            # ...
            # ret <rd>
            mov_ret_errors = []
            nop_spacer_errors = []
            missing_asm_annot_errors = []
            c_func_br_errors = []
            ldp_spacer_error_funcs = set([])
            stp_spacer_error_funcs = set([])
            ldp_spacer_errors = []
            stp_spacer_errors = []
            atomic_prologue_errors = []
            atomic_prologue_error_funcs = set([])
            for func_i, func, lines, last_insns in objdump_uninstr.each_func_lines(num_last_insns=2, with_func_i=True):
                mov_registers = set([])
                ret_registers = set([])
                is_c_func = func in c_functions
                saw_br = False
                #if objdump_uninstr.JOPP_FUNCTION_NOP_SPACERS and \
                        #not instrument.skip_func(func, skip, skip_asm) and func in asm_functions:
                    #if any(not re.search('\tnop$', l) for l in last_insns if l is not None):
                        #nop_spacer_errors.append(lines)
                for i, line in enumerate(lines, start=func_i):
                    def slice_lines(start, end):
                        return lines[start-func_i:end-func_i]
                    m = re.search(r'mov\t(?P<mov_register>{register_re}), x30'.format(register_re=register_re), line)
                    if m and m.group('mov_register') != 'sp':
                        mov_registers.add(m.group('mov_register'))
                        continue
                    m = re.search(r'ret\t(?P<ret_register>{register_re})'.format(register_re=register_re), line)
                    if m:
                        ret_registers.add(m.group('ret_register'))
                        continue
                    m = re.search(r'ldp\tx29,\s+x30,', line)
                    if m:
                        for l in lines[i+1:i+3]:
                            if not re.search(r'nop$'):
                                ldp_spacer_errors.append(lines)
                                ldp_spacer_error_funcs.add(func)
                                break
                        continue
                    m = re.search(r'stp\tx29,\s+x30,', line)
                    if m and func not in skip_stp:
                        missing_nop = False
                        for l in slice_lines(i-1, i):
                            if not re.search(r'nop$', l):
                                stp_spacer_errors.append(lines)
                                stp_spacer_error_funcs.add(func)
                                missing_nop = True
                                break
                        if missing_nop:
                            continue
                        if func == '__kvm_vcpu_run':
                            pr({'func':func})
                        mov_j, movx29_insn = instrument.find_add_x29_x30_imm(objdump_uninstr, func, func_i, i)
                        for l in slice_lines(i+1, mov_j):
                            if func not in atomic_prologue_error_funcs and re.search(r'\b(x29|sp)\b', insn_text(l)):
                                atomic_prologue_errors.append(lines)
                                atomic_prologue_error_funcs.add(func)
                                break
                        continue
                # End of function; check for errors in that function, and if so, perserve its output.
                if len(mov_registers.intersection(ret_registers)) > 0 and func not in skip_save_lr_to_stack:
                    mov_ret_errors.append(lines)

            errmsg(c_func_br_errors, """
            Saw a C function in vmlinux without information about the number of arguments it takes.

            We need to know this to zero registers on BLR jumps.
            """)

            errmsg(missing_asm_annot_errors, """
            Saw an assembly routine(s) that hasn't been annotated with the number of
            general purpose registers it uses.

            Change ENTRY to FUNC_ENTRY for these assembly functions.
            """)

            errmsg(nop_spacer_errors, """
            Saw an assembly routine(s) that doesn't have 2 nop instruction immediately
            before the function label.

            We need these for any function that might be the target of a blr instruction!
            """)

            errmsg(mov_ret_errors, """
            Saw an assembly routine(s) saving LR into a register instead of on the stack.
            This would prevent us from re-encrypting it properly!
            Modify these routine(s) to save LR on the stack and adjust the frame pointer (like in prologues of C functions).
            e.g.
            stp	x29, x30, [sp,#-16]!
            mov	x29, sp
            ...
            ldp	x29, x30, [sp],#16
            ret

            NOTE: We're only reporting functions found in the compiled vmlinux
            (gcc might remove dead code that needs patching as well)
            """)
            errmsg(ldp_spacer_errors, """
            Saw a function with ldp x29, x30 but without 2 nops following it.
            Either add an LDP_SPACER to this, use the right compiler, or make an exception.
            """)
            errmsg(stp_spacer_errors, """
            Saw a function with stp x29, x30 but without 1 nop before it.
            Either add an STP_SPACER to this, use the right compiler, or make an exception.
            """)
            warmsg(atomic_prologue_errors, """
            Saw a function prologue with:
            <func>:
                stp x29, x30, ...
                (insns)
                add x29, sp, #...

            BUT, one of the "(insns)" mentions either x29 or sp, so it might not be safe to turn this into:

            <func>:
                stp x29, x30, ...
                add x29, sp, #...
                (insns)
            """)

    procs = []
    # for validate in [validate_uninstr_lines]:
    for validate in [validate_bin, validate_instr, validate_uninstr_lines, validate_uninstr_binary]:
        if threads == 1:
            validate()
            continue
        proc = multiprocessing.Process(target=validate, args=())
        proc.start()
        procs.append(proc)
    for proc in procs:
        proc.join()

    return bool(success.value)

if common.run_from_ipython():
    def _x(*hexints):
        xored = 0
        for hexint in hexints:
            xored ^= hexint
        return "0x{0:x}".format(xored)

    def _d(*addrs):
        """
        Assume key is like
        0x1111111111111111
        Guess key, then decrypt used guessed key.
        """
        def __d(addr):
            addr = re.sub('^0x', '', addr)
            first_4bits = int(addr[0], 16)
            first_byte_of_key = (0xf ^ first_4bits) << 4 | (0xf ^ first_4bits)
            key = 0
            for i in xrange(0, 8):
                key |= first_byte_of_key << i*8
            return {'decaddr':'0x' + instrument._hex(instrument._int(addr) ^ key),
                    'key':'0x' + instrument._hex(key)}
        return map(__d, addrs)
