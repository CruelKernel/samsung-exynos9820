#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Module IntegrityRoutine Contains IntegrityRoutine class helps with FIPS 140-2 build time integrity routine.
This module is needed to calculate HMAC and embed other needed stuff.
"""

import hmac
import hashlib
import bisect
import itertools
import binascii
from ELF import *

__author__ = "Vadym Stupakov"
__copyright__ = "Copyright (c) 2017 Samsung Electronics"
__credits__ = ["Vadym Stupakov"]
__version__ = "1.0"
__maintainer__ = "Vadym Stupakov"
__email__ = "v.stupakov@samsung.com"
__status__ = "Production"


class IntegrityRoutine(ELF):
    """
    Utils for fips-integrity process
    """
    def __init__(self, elf_file, readelf_path="readelf"):
        ELF.__init__(self, elf_file, readelf_path)

    @staticmethod
    def __remove_all_dublicates(lst):
        """
        Removes all occurrences of tha same value. For instance: transforms [1, 2, 3, 1] -> [2, 3]
        :param lst: input list
        :return: lst w/o duplicates
        """
        to_remove = list()
        for i in range(len(lst)):
            it = itertools.islice(lst, i + 1, len(lst) - 1, None)
            for j, val in enumerate(it, start=i+1):
                if val == lst[i]:
                    to_remove.extend([lst[i], lst[j]])

        for el in to_remove:
            lst.remove(el)

    def get_reloc_gaps(self, start_addr, end_addr):
        """
        :param start_addr: start address :int
        :param end_addr: end address: int
        :returns list of relocation gaps like [[gap_start, gap_end], [gap_start, gap_end], ...]
        """
        all_relocs = self.get_relocs(start_addr, end_addr)
        relocs_gaps = list()
        for addr in all_relocs:
            relocs_gaps.append(addr)
            relocs_gaps.append(addr + 8)
        self.__remove_all_dublicates(relocs_gaps)
        relocs_gaps = [[addr1, addr2] for addr1, addr2 in self.utils.pairwise(relocs_gaps)]
        return relocs_gaps

    def get_altinstruction_gaps(self, start_addr, end_addr):
        """
        :param start_addr: start address :int
        :param end_addr: end address: int
        :returns list of relocation gaps like [[gap_start, gap_end], [gap_start, gap_end], ...]
        """
        all_altinstr = self.get_altinstructions(start_addr, end_addr)
        altinstr_gaps = list()
        for addr in all_altinstr:
            altinstr_gaps.append(addr)
            altinstr_gaps.append(addr + 4)
        self.__remove_all_dublicates(altinstr_gaps)
        altinstr_gaps = [[addr1, addr2] for addr1, addr2 in self.utils.pairwise(altinstr_gaps)]
        return altinstr_gaps

    def get_addrs_for_hmac(self, sec_sym_sequence, gaps=None):
        """
        Generate addresses for calculating HMAC
        :param sec_sym_sequence: [addr_start1, addr_end1, ..., addr_startN, addr_endN],
        :param gaps: [[start_gap_addr, end_gap_addr], [start_gap_addr, end_gap_addr]]
        :return: addresses for calculating HMAC: [[addr_start, addr_end], [addr_start, addr_end], ...]
        """
        addrs_for_hmac = list()

        if gaps == None:
            return [[0, 0]]

        for section_name, sym_names in sec_sym_sequence.items():
            for symbol in self.get_symbol_by_name(sym_names):
                addrs_for_hmac.append(symbol.addr)
        addrs_for_hmac.extend(self.utils.flatten(gaps))
        addrs_for_hmac.sort()
        return [[item1, item2] for item1, item2 in self.utils.pairwise(addrs_for_hmac)]

    def embed_bytes(self, vaddr, in_bytes):
        """
        Write bytes to ELF file
        :param vaddr: virtual address in ELF
        :param in_bytes: byte array to write
        """
        offset = self.vaddr_to_file_offset(vaddr)
        with open(self.get_elf_file(), "rb+") as elf_file:
            elf_file.seek(offset)
            elf_file.write(in_bytes)

    def __update_hmac(self, hmac_obj, file_obj, file_offset_start, file_offset_end):
        """
        Update hmac from addrstart tp addr_end
        FIXMI: it needs to implement this function via fixed block size
        :param file_offset_start: could be string or int
        :param file_offset_end:   could be string or int
        """
        file_offset_start = self.utils.to_int(file_offset_start)
        file_offset_end = self.utils.to_int(file_offset_end)
        file_obj.seek(self.vaddr_to_file_offset(file_offset_start))
        block_size = file_offset_end - file_offset_start
        msg = file_obj.read(block_size)
        hmac_obj.update(msg)

    def get_hmac(self, offset_sequence, key, output_type="byte"):
        """
        Calculate HMAC
        :param offset_sequence: start and end addresses sequence [addr_start, addr_end], [addr_start, addr_end], ...]
        :param key HMAC key: string value
        :param output_type string value. Could be "hex" or "byte"
        :return: bytearray or hex string
        """
        digest = hmac.new(bytearray(key.encode("utf-8")), digestmod=hashlib.sha256)
        with open(self.get_elf_file(), "rb") as file:
            for addr_start, addr_end in offset_sequence:
                self.__update_hmac(digest, file, addr_start, addr_end)
        if output_type == "byte":
            return digest.digest()
        if output_type == "hex":
            return digest.hexdigest()

    def __find_nearest_symbol_by_vaddr(self, vaddr, method):
        """
        Find nearest symbol near vaddr
        :param vaddr:
        :return: idx of symbol from self.get_symbols()
        """
        symbol = self.get_symbol_by_vaddr(vaddr)
        if symbol is None:
            raise ValueError("Can't find symbol by vaddr")
        idx = method(list(self.get_symbols()), vaddr)
        return idx

    def find_rnearest_symbol_by_vaddr(self, vaddr):
        """
        Find right nearest symbol near vaddr
        :param vaddr:
        :return: idx of symbol from self.get_symbols()
        """
        return self.__find_nearest_symbol_by_vaddr(vaddr, bisect.bisect_right)

    def find_lnearest_symbol_by_vaddr(self, vaddr):
        """
        Find left nearest symbol near vaddr
        :param vaddr:
        :return: idx of symbol from self.get_symbols()
        """
        return self.__find_nearest_symbol_by_vaddr(vaddr, bisect.bisect_left)

    def find_symbols_between_vaddrs(self, vaddr_start, vaddr_end):
        """
        Returns list of symbols between two virtual addresses
        :param vaddr_start:
        :param vaddr_end:
        :return: [(Symbol(), Section)]
        """
        symbol_start = self.get_symbol_by_vaddr(vaddr_start)
        symbol_end = self.get_symbol_by_vaddr(vaddr_end)
        if symbol_start is None or symbol_end is None:
            raise ValueError("Error: Cannot find symbol by vaddr. vaddr should coincide with symbol address!")

        idx_start = self.find_lnearest_symbol_by_vaddr(vaddr_start)
        idx_end = self.find_lnearest_symbol_by_vaddr(vaddr_end)

        sym_sec = list()
        for idx in range(idx_start, idx_end):
            symbol_addr = list(self.get_symbols())[idx]
            symbol = self.get_symbol_by_vaddr(symbol_addr)
            section = self.get_section_by_vaddr(symbol_addr)
            sym_sec.append((symbol, section))

        sym_sec.sort(key=lambda x: x[0])
        return sym_sec

    @staticmethod
    def __get_skipped_bytes(symbol, relocs):
        """
        :param symbol: Symbol()
        :param relocs: [[start1, end1], [start2, end2]]
        :return: Returns skipped bytes and [[start, end]] addresses that show which bytes were skipped
        """
        symbol_start_addr = symbol.addr
        symbol_end_addr = symbol.addr + symbol.size
        skipped_bytes = 0
        reloc_addrs = list()
        for reloc_start, reloc_end in relocs:
            if reloc_start >= symbol_start_addr and reloc_end <= symbol_end_addr:
                skipped_bytes += reloc_end - reloc_start
                reloc_addrs.append([reloc_start, reloc_end])
            if reloc_start > symbol_end_addr:
                break

        return skipped_bytes, reloc_addrs

    def print_covered_info(self, sec_sym, relocs, print_reloc_addrs=False, sort_by="address", reverse=False):
        """
        Prints information about covered symbols in detailed table:
        |N| symbol name | symbol address     | symbol section | bytes skipped | skipped bytes address range      |
        |1| symbol      | 0xXXXXXXXXXXXXXXXX | .rodata        | 8             | [[addr1, addr2], [addr1, addr2]] |
        :param sec_sym: {section_name : [sym_name1, sym_name2]}
        :param relocs: [[start1, end1], [start2, end2]]
        :param print_reloc_addrs: print or not skipped bytes address range
        :param sort_by: method for sorting table. Could be: "address", "name", "section"
        :param reverse: sort order
        """
        if sort_by.lower() == "address":
            def sort_method(x): return x[0].addr
        elif sort_by.lower() == "name":
            def sort_method(x): return x[0].name
        elif sort_by.lower() == "section":
            def sort_method(x): return x[1].name
        else:
            raise ValueError("Invalid sort type!")
        table_format = "|{:4}| {:50} | {:18} | {:20} | {:15} |"
        if print_reloc_addrs is True:
            table_format += "{:32} |"

        print(table_format.format("N", "symbol name", "symbol address", "symbol section", "bytes skipped",
                                  "skipped bytes address range"))
        data_to_print = list()
        for sec_name, sym_names in sec_sym.items():
            for symbol_start, symbol_end in self.utils.pairwise(self.get_symbol_by_name(sym_names)):
                symbol_sec_in_range = self.find_symbols_between_vaddrs(symbol_start.addr, symbol_end.addr)
                for symbol, section in symbol_sec_in_range:
                    skipped_bytes, reloc_addrs = self.__get_skipped_bytes(symbol, relocs)
                    reloc_addrs_str = "["
                    for start_addr, end_addr in reloc_addrs:
                        reloc_addrs_str += "[{}, {}], ".format(hex(start_addr), hex(end_addr))
                    reloc_addrs_str += "]"
                    if symbol.size > 0:
                        data_to_print.append((symbol, section, skipped_bytes, reloc_addrs_str))

        skipped_bytes_size = 0
        symbol_covered_size = 0
        cnt = 0
        data_to_print.sort(key=sort_method, reverse=reverse)
        for symbol, section, skipped_bytes, reloc_addrs_str in data_to_print:
            cnt += 1
            symbol_covered_size += symbol.size
            skipped_bytes_size += skipped_bytes
            if print_reloc_addrs is True:
                print(table_format.format(cnt, symbol.name, hex(symbol.addr), section.name,
                                          self.utils.human_size(skipped_bytes), reloc_addrs_str))
            else:
                print(table_format.format(cnt, symbol.name, hex(symbol.addr), section.name,
                                          self.utils.human_size(skipped_bytes)))
        addrs_for_hmac = self.get_addrs_for_hmac(sec_sym, relocs)
        all_covered_size = 0
        for addr_start, addr_end in addrs_for_hmac:
            all_covered_size += addr_end - addr_start
        print("Symbol covered bytes len: {} ".format(self.utils.human_size(symbol_covered_size - skipped_bytes_size)))
        print("All covered bytes len   : {} ".format(self.utils.human_size(all_covered_size)))
        print("Skipped bytes len       : {} ".format(self.utils.human_size(skipped_bytes_size)))

    def dump_covered_bytes(self, vaddr_seq, out_file):
        """
        Dumps covered bytes
        :param vaddr_seq: [[start1, end1], [start2, end2]] start - end sequence of covered bytes
        :param out_file: file where will be stored dumped bytes
        """
        with open(self.get_elf_file(), "rb") as elf_fp:
            with open(out_file, "wb") as out_fp:
                for vaddr_start, vaddr_end, in vaddr_seq:
                    elf_fp.seek(self.vaddr_to_file_offset(vaddr_start))
                    out_fp.write(elf_fp.read(vaddr_end - vaddr_start))

    def make_integrity(self, sec_sym, module_name, debug=False, print_reloc_addrs=False, sort_by="address",
                       reverse=False):
        """
        Calculate HMAC and embed needed info
        :param sec_sym: {sec_name: [addr1, addr2, ..., addrN]}
        :param module_name: module name that you want to make integrity. See Makefile targets
        :param debug: If True prints debug information
        :param print_reloc_addrs: If True, print relocation addresses that are skipped
        :param sort_by: sort method
        :param reverse: sort order
        
        Checks: .rodata     section for relocations
                .text       section for alternated instructions
                .init.text  section for alternated instructions
        """
        rel_addr_start = self.get_symbol_by_name("first_" + module_name + "_rodata")
        rel_addr_end = self.get_symbol_by_name("last_" + module_name + "_rodata")
        text_addr_start = self.get_symbol_by_name("first_" + module_name + "_text")
        text_addr_end = self.get_symbol_by_name("last_" + module_name + "_text")
        init_addr_start = self.get_symbol_by_name("first_" + module_name + "_init")
        init_addr_end = self.get_symbol_by_name("last_" + module_name + "_init")

        gaps = self.get_reloc_gaps(rel_addr_start.addr, rel_addr_end.addr)
        gaps.extend(self.get_altinstruction_gaps(text_addr_start.addr, text_addr_end.addr))
        gaps.extend(self.get_altinstruction_gaps(init_addr_start.addr, init_addr_end.addr))
        gaps.sort()

        addrs_for_hmac = self.get_addrs_for_hmac(sec_sym, gaps)

        digest = self.get_hmac(addrs_for_hmac, "The quick brown fox jumps over the lazy dog")

        self.embed_bytes(self.get_symbol_by_name("builtime_" + module_name + "_hmac").addr,
                         self.utils.to_bytearray(digest))

        self.embed_bytes(self.get_symbol_by_name("integrity_" + module_name + "_addrs").addr,
                         self.utils.to_bytearray(addrs_for_hmac))

        self.embed_bytes(self.get_symbol_by_name(module_name + "_buildtime_address").addr,
                        self.utils.to_bytearray(self.get_symbol_by_name(module_name + "_buildtime_address").addr))

        print("HMAC for \"{}\" module is: {}".format(module_name, binascii.hexlify(digest)))
        if debug:
            self.print_covered_info(sec_sym, gaps, print_reloc_addrs=print_reloc_addrs, sort_by=sort_by,
                                    reverse=reverse)
            self.dump_covered_bytes(addrs_for_hmac, "covered_dump_for_" + module_name + ".bin")

        print("FIPS integrity procedure has been finished for {}".format(module_name))
