# Copyright (C) 2021-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import gdb
import gdb.disassembler
import struct
import sys

from gdb.disassembler import Disassembler, DisassemblerResult

# A global, holds the program-counter address at which we should
# perform the extra disassembly that this script provides.
current_pc = None


def builtin_disassemble_wrapper(info):
    result = gdb.disassembler.builtin_disassemble(info)
    assert result.length > 0
    assert len(result.parts) > 0
    tmp_str = ""
    for p in result.parts:
        assert p.string == str(p)
        tmp_str += p.string
    assert tmp_str == result.string
    return result


def check_building_disassemble_result():
    """Check that we can create DisassembleResult objects correctly."""

    result = gdb.disassembler.DisassemblerResult()

    print("PASS")


def is_nop(s):
    return s == "nop" or s == "nop\t0"


# Remove all currently registered disassemblers.
def remove_all_python_disassemblers():
    for a in gdb.architecture_names():
        gdb.disassembler.register_disassembler(None, a)
    gdb.disassembler.register_disassembler(None, None)


class TestDisassembler(Disassembler):
    """A base class for disassemblers within this script to inherit from.
    Implements the __call__ method and ensures we only do any
    disassembly wrapping for the global CURRENT_PC."""

    def __init__(self):
        global current_pc

        super().__init__("TestDisassembler")
        self.__info = None
        if current_pc == None:
            raise gdb.GdbError("no current_pc set")

    def __call__(self, info):
        global current_pc

        if info.address != current_pc:
            return None
        self.__info = info
        return self.disassemble(info)

    def get_info(self):
        return self.__info

    def disassemble(self, info):
        raise NotImplementedError("override the disassemble method")


class ShowInfoRepr(TestDisassembler):
    """Call the __repr__ method on the DisassembleInfo, convert the result
    to a string, and incude it in a comment in the disassembler output."""

    def disassemble(self, info):
        comment = "\t## " + repr(info)
        result = builtin_disassemble_wrapper(info)
        string = result.string + comment
        length = result.length
        return DisassemblerResult(length=length, string=string)


class ShowInfoSubClassRepr(TestDisassembler):
    """Create a sub-class of DisassembleInfo.  Create an instace of this
    sub-class and call the __repr__ method on it.  Convert the result
    to a string, and incude it in a comment in the disassembler
    output.  The DisassembleInfo sub-class does not override __repr__
    so we are calling the implementation on the parent class."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        """A wrapper around DisassembleInfo, doesn't add any new
        functionality, just gives a new name in order to check the
        __repr__ functionality."""

        def __init__(self, info):
            super().__init__(info)

    def disassemble(self, info):
        info = self.MyInfo(info)
        comment = "\t## " + repr(info)
        result = builtin_disassemble_wrapper(info)
        string = result.string + comment
        length = result.length
        return DisassemblerResult(length=length, string=string)


class ShowResultRepr(TestDisassembler):
    """Call the __repr__ method on the DisassemblerResult, convert the
    result to a string, and incude it in a comment in the disassembler
    output."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        comment = "\t## " + repr(result)
        string = result.string + comment
        length = result.length
        return DisassemblerResult(length=length, string=string)


class ShowResultStr(TestDisassembler):
    """Call the __str__ method on a DisassemblerResult object, incude the
    resulting string in a comment within the disassembler output."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        comment = "\t## " + str(result)
        string = result.string + comment
        length = result.length
        return DisassemblerResult(length=length, string=string, parts=None)


class GlobalPreInfoDisassembler(TestDisassembler):
    """Check the attributes of DisassembleInfo before disassembly has occurred."""

    def disassemble(self, info):
        ad = info.address
        ar = info.architecture

        if ad != current_pc:
            raise gdb.GdbError("invalid address")

        if not isinstance(ar, gdb.Architecture):
            raise gdb.GdbError("invalid architecture type")

        result = builtin_disassemble_wrapper(info)

        text = result.string + "\t## ad = 0x%x, ar = %s" % (ad, ar.name())
        return DisassemblerResult(result.length, text)


class GlobalPostInfoDisassembler(TestDisassembler):
    """Check the attributes of DisassembleInfo after disassembly has occurred."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)

        ad = info.address
        ar = info.architecture

        if ad != current_pc:
            raise gdb.GdbError("invalid address")

        if not isinstance(ar, gdb.Architecture):
            raise gdb.GdbError("invalid architecture type")

        text = result.string + "\t## ad = 0x%x, ar = %s" % (ad, ar.name())
        return DisassemblerResult(result.length, text)


class GlobalReadDisassembler(TestDisassembler):
    """Check the DisassembleInfo.read_memory method.  Calls the builtin
    disassembler, then reads all of the bytes of this instruction, and
    adds them as a comment to the disassembler output."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        len = result.length
        str = ""
        for o in range(len):
            if str != "":
                str += " "
            v = bytes(info.read_memory(1, o))[0]
            if sys.version_info[0] < 3:
                v = struct.unpack("<B", v)
            str += "0x%02x" % v
        text = result.string + "\t## bytes = %s" % str
        return DisassemblerResult(result.length, text)


class GlobalAddrDisassembler(TestDisassembler):
    """Check the gdb.format_address method."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        arch = info.architecture
        addr = info.address
        program_space = info.progspace
        str = gdb.format_address(addr, program_space, arch)
        text = result.string + "\t## addr = %s" % str
        return DisassemblerResult(result.length, text)


class GdbErrorEarlyDisassembler(TestDisassembler):
    """Raise a GdbError instead of performing any disassembly."""

    def disassemble(self, info):
        raise gdb.GdbError("GdbError instead of a result")


class RuntimeErrorEarlyDisassembler(TestDisassembler):
    """Raise a RuntimeError instead of performing any disassembly."""

    def disassemble(self, info):
        raise RuntimeError("RuntimeError instead of a result")


class GdbErrorLateDisassembler(TestDisassembler):
    """Raise a GdbError after calling the builtin disassembler."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        raise gdb.GdbError("GdbError after builtin disassembler")


class RuntimeErrorLateDisassembler(TestDisassembler):
    """Raise a RuntimeError after calling the builtin disassembler."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        raise RuntimeError("RuntimeError after builtin disassembler")


class MemoryErrorEarlyDisassembler(TestDisassembler):
    """Throw a memory error, ignore the error and disassemble."""

    def disassemble(self, info):
        tag = "## FAIL"
        try:
            info.read_memory(1, -info.address + 2)
        except gdb.MemoryError:
            tag = "## AFTER ERROR"
        result = builtin_disassemble_wrapper(info)
        text = result.string + "\t" + tag
        return DisassemblerResult(result.length, text)


class MemoryErrorLateDisassembler(TestDisassembler):
    """Throw a memory error after calling the builtin disassembler, but
    before we return a result."""

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        # The following read will throw an error.
        info.read_memory(1, -info.address + 2)
        return DisassemblerResult(1, "BAD")


class RethrowMemoryErrorDisassembler(TestDisassembler):
    """Catch and rethrow a memory error."""

    def disassemble(self, info):
        try:
            info.read_memory(1, -info.address + 2)
        except gdb.MemoryError as e:
            raise gdb.MemoryError("cannot read code at address 0x2")
        return DisassemblerResult(1, "BAD")


class ResultOfWrongType(TestDisassembler):
    """Return something that is not a DisassemblerResult from disassemble method"""

    class Blah:
        def __init__(self, length, string):
            self.length = length
            self.string = string

    def disassemble(self, info):
        return self.Blah(1, "ABC")


class TaggingDisassembler(TestDisassembler):
    """A simple disassembler that just tags the output."""

    def __init__(self, tag):
        super().__init__()
        self._tag = tag

    def disassemble(self, info):
        result = builtin_disassemble_wrapper(info)
        text = result.string + "\t## tag = %s" % self._tag
        return DisassemblerResult(result.length, text)


class GlobalCachingDisassembler(TestDisassembler):
    """A disassembler that caches the DisassembleInfo that is passed in,
    as well as a copy of the original DisassembleInfo.

    Once the call into the disassembler is complete then the
    DisassembleInfo objects become invalid, and any calls into them
    should trigger an exception."""

    # This is where we cache the DisassembleInfo objects.
    cached_insn_disas = []

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

    def disassemble(self, info):
        """Disassemble the instruction, add a CACHED comment to the output,
        and cache the DisassembleInfo so that it is not garbage collected."""
        GlobalCachingDisassembler.cached_insn_disas.append(info)
        GlobalCachingDisassembler.cached_insn_disas.append(self.MyInfo(info))
        result = builtin_disassemble_wrapper(info)
        text = result.string + "\t## CACHED"
        return DisassemblerResult(result.length, text)

    @staticmethod
    def check():
        """Check that all of the methods on the cached DisassembleInfo trigger an
        exception."""
        for info in GlobalCachingDisassembler.cached_insn_disas:
            assert isinstance(info, gdb.disassembler.DisassembleInfo)
            assert not info.is_valid()
            try:
                val = info.address
                raise gdb.GdbError("DisassembleInfo.address is still valid")
            except RuntimeError as e:
                assert str(e) == "DisassembleInfo is no longer valid."
            except:
                raise gdb.GdbError(
                    "DisassembleInfo.address raised an unexpected exception"
                )

            try:
                val = info.architecture
                raise gdb.GdbError("DisassembleInfo.architecture is still valid")
            except RuntimeError as e:
                assert str(e) == "DisassembleInfo is no longer valid."
            except:
                raise gdb.GdbError(
                    "DisassembleInfo.architecture raised an unexpected exception"
                )

            try:
                val = info.read_memory(1, 0)
                raise gdb.GdbError("DisassembleInfo.read is still valid")
            except RuntimeError as e:
                assert str(e) == "DisassembleInfo is no longer valid."
            except:
                raise gdb.GdbError(
                    "DisassembleInfo.read raised an unexpected exception"
                )

        print("PASS")


class GlobalNullDisassembler(TestDisassembler):
    """A disassembler that does not change the output at all."""

    def disassemble(self, info):
        pass


class ReadMemoryMemoryErrorDisassembler(TestDisassembler):
    """Raise a MemoryError exception from the DisassembleInfo.read_memory
    method."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            # Throw a memory error with a specific address.  We don't
            # expect this address to show up in the output though.
            raise gdb.MemoryError(0x1234)

    def disassemble(self, info):
        info = self.MyInfo(info)
        return builtin_disassemble_wrapper(info)


class ReadMemoryGdbErrorDisassembler(TestDisassembler):
    """Raise a GdbError exception from the DisassembleInfo.read_memory
    method."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            raise gdb.GdbError("read_memory raised GdbError")

    def disassemble(self, info):
        info = self.MyInfo(info)
        return builtin_disassemble_wrapper(info)


class ReadMemoryRuntimeErrorDisassembler(TestDisassembler):
    """Raise a RuntimeError exception from the DisassembleInfo.read_memory
    method."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            raise RuntimeError("read_memory raised RuntimeError")

    def disassemble(self, info):
        info = self.MyInfo(info)
        return builtin_disassemble_wrapper(info)


class ReadMemoryCaughtMemoryErrorDisassembler(TestDisassembler):
    """Raise a MemoryError exception from the DisassembleInfo.read_memory
    method, catch this in the outer disassembler."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            raise gdb.MemoryError(0x1234)

    def disassemble(self, info):
        info = self.MyInfo(info)
        try:
            return builtin_disassemble_wrapper(info)
        except gdb.MemoryError:
            return None


class ReadMemoryCaughtGdbErrorDisassembler(TestDisassembler):
    """Raise a GdbError exception from the DisassembleInfo.read_memory
    method, catch this in the outer disassembler."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            raise gdb.GdbError("exception message")

    def disassemble(self, info):
        info = self.MyInfo(info)
        try:
            return builtin_disassemble_wrapper(info)
        except gdb.GdbError as e:
            if e.args[0] == "exception message":
                return None
            raise e


class ReadMemoryCaughtRuntimeErrorDisassembler(TestDisassembler):
    """Raise a RuntimeError exception from the DisassembleInfo.read_memory
    method, catch this in the outer disassembler."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            raise RuntimeError("exception message")

    def disassemble(self, info):
        info = self.MyInfo(info)
        try:
            return builtin_disassemble_wrapper(info)
        except RuntimeError as e:
            if e.args[0] == "exception message":
                return None
            raise e


class MemorySourceNotABufferDisassembler(TestDisassembler):
    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            return 1234

    def disassemble(self, info):
        info = self.MyInfo(info)
        return builtin_disassemble_wrapper(info)


class MemorySourceBufferTooLongDisassembler(TestDisassembler):
    """The read memory returns too many bytes."""

    class MyInfo(gdb.disassembler.DisassembleInfo):
        def __init__(self, info):
            super().__init__(info)

        def read_memory(self, length, offset):
            buffer = super().read_memory(length, offset)
            # Create a new memory view made by duplicating BUFFER.  This
            # will trigger an error as GDB expects a buffer of exactly
            # LENGTH to be returned, while this will return a buffer of
            # 2*LENGTH.
            return memoryview(
                bytes([int.from_bytes(x, "little") for x in (list(buffer[0:]) * 2)])
            )

    def disassemble(self, info):
        info = self.MyInfo(info)
        return builtin_disassemble_wrapper(info)


class ErrorCreatingTextPart_NoArgs(TestDisassembler):
    """Try to create a DisassemblerTextPart with no arguments."""

    def disassemble(self, info):
        part = info.text_part()
        return None


class ErrorCreatingAddressPart_NoArgs(TestDisassembler):
    """Try to create a DisassemblerAddressPart with no arguments."""

    def disassemble(self, info):
        part = info.address_part()
        return None


class ErrorCreatingTextPart_NoString(TestDisassembler):
    """Try to create a DisassemblerTextPart with no string argument."""

    def disassemble(self, info):
        part = info.text_part(gdb.disassembler.STYLE_TEXT)
        return None


class ErrorCreatingTextPart_NoStyle(TestDisassembler):
    """Try to create a DisassemblerTextPart with no string argument."""

    def disassemble(self, info):
        part = info.text_part(string="abc")
        return None


class ErrorCreatingTextPart_StringAndParts(TestDisassembler):
    """Try to create a DisassemblerTextPart with both a string and a parts list."""

    def disassemble(self, info):
        parts = []
        parts.append(info.text_part(gdb.disassembler.STYLE_TEXT, "p1"))
        parts.append(info.text_part(gdb.disassembler.STYLE_TEXT, "p2"))

        return DisassemblerResult(length=4, string="p1p2", parts=parts)


class All_Text_Part_Styles(TestDisassembler):
    """Create text parts with all styles."""

    def disassemble(self, info):
        parts = []
        parts.append(info.text_part(gdb.disassembler.STYLE_TEXT, "p1"))
        parts.append(info.text_part(gdb.disassembler.STYLE_MNEMONIC, "p2"))
        parts.append(info.text_part(gdb.disassembler.STYLE_SUB_MNEMONIC, "p3"))
        parts.append(info.text_part(gdb.disassembler.STYLE_ASSEMBLER_DIRECTIVE, "p4"))
        parts.append(info.text_part(gdb.disassembler.STYLE_REGISTER, "p5"))
        parts.append(info.text_part(gdb.disassembler.STYLE_IMMEDIATE, "p6"))
        parts.append(info.text_part(gdb.disassembler.STYLE_ADDRESS, "p7"))
        parts.append(info.text_part(gdb.disassembler.STYLE_ADDRESS_OFFSET, "p8"))
        parts.append(info.text_part(gdb.disassembler.STYLE_SYMBOL, "p9"))
        parts.append(info.text_part(gdb.disassembler.STYLE_COMMENT_START, "p10"))

        result = builtin_disassemble_wrapper(info)
        result = DisassemblerResult(length=result.length, parts=parts)

        tmp_str = ""
        for p in parts:
            assert p.string == str(p)
            tmp_str += str(p)
        assert tmp_str == result.string

        return result


class Build_Result_Using_All_Parts(TestDisassembler):
    """Disassemble an instruction and return a result that makes use of
    text and address parts."""

    def disassemble(self, info):
        global current_pc

        parts = []
        parts.append(info.text_part(gdb.disassembler.STYLE_MNEMONIC, "fake"))
        parts.append(info.text_part(gdb.disassembler.STYLE_TEXT, "\t"))
        parts.append(info.text_part(gdb.disassembler.STYLE_REGISTER, "reg"))
        parts.append(info.text_part(gdb.disassembler.STYLE_TEXT, ", "))
        addr_part = info.address_part(current_pc)
        assert addr_part.address == current_pc
        parts.append(addr_part)
        parts.append(info.text_part(gdb.disassembler.STYLE_TEXT, ", "))
        parts.append(info.text_part(gdb.disassembler.STYLE_IMMEDIATE, "123"))

        result = builtin_disassemble_wrapper(info)
        result = DisassemblerResult(length=result.length, parts=parts)
        return result


class BuiltinDisassembler(Disassembler):
    """Just calls the builtin disassembler."""

    def __init__(self):
        super().__init__("BuiltinDisassembler")

    def __call__(self, info):
        return builtin_disassemble_wrapper(info)


class AnalyzingDisassembler(Disassembler):
    class MyInfo(gdb.disassembler.DisassembleInfo):
        """Wrapper around builtin DisassembleInfo type that overrides the
        read_memory method."""

        def __init__(self, info, start, end, nop_bytes):
            """INFO is the DisassembleInfo we are wrapping.  START and END are
            addresses, and NOP_BYTES should be a memoryview object.

            The length (END - START) should be the same as the length
            of NOP_BYTES.

            Any memory read requests outside the START->END range are
            serviced normally, but any attempt to read within the
            START->END range will return content from NOP_BYTES."""
            super().__init__(info)
            self._start = start
            self._end = end
            self._nop_bytes = nop_bytes

        def _read_replacement(self, length, offset):
            """Return a slice of the buffer representing the replacement nop
            instructions."""

            assert self._nop_bytes is not None
            rb = self._nop_bytes

            # If this request is outside of a nop instruction then we don't know
            # what to do, so just raise a memory error.
            if offset >= len(rb) or (offset + length) > len(rb):
                raise gdb.MemoryError("invalid length and offset combination")

            # Return only the slice of the nop instruction as requested.
            s = offset
            e = offset + length
            return rb[s:e]

        def read_memory(self, length, offset=0):
            """Callback used by the builtin disassembler to read the contents of
            memory."""

            # If this request is within the region we are replacing with 'nop'
            # instructions, then call the helper function to perform that
            # replacement.
            if self._start is not None:
                assert self._end is not None
                if self.address >= self._start and self.address < self._end:
                    return self._read_replacement(length, offset)

            # Otherwise, we just forward this request to the default read memory
            # implementation.
            return super().read_memory(length, offset)

    def __init__(self):
        """Constructor."""
        super().__init__("AnalyzingDisassembler")

        # Details about the instructions found during the first disassembler
        # pass.
        self._pass_1_length = []
        self._pass_1_insn = []
        self._pass_1_address = []

        # The start and end address for the instruction we will replace with
        # one or more 'nop' instructions during pass two.
        self._start = None
        self._end = None

        # The index in the _pass_1_* lists for where the nop instruction can
        # be found, also, the buffer of bytes that make up a nop instruction.
        self._nop_index = None
        self._nop_bytes = None

        # A flag that indicates if we are in the first or second pass of
        # this disassembler test.
        self._first_pass = True

        # The disassembled instructions collected during the second pass.
        self._pass_2_insn = []

        # A copy of _pass_1_insn that has been modified to include the extra
        # 'nop' instructions we plan to insert during the second pass.  This
        # is then checked against _pass_2_insn after the second disassembler
        # pass has completed.
        self._check = []

    def __call__(self, info):
        """Called to perform the disassembly."""

        # Override the info object, this provides access to our
        # read_memory function.
        info = self.MyInfo(info, self._start, self._end, self._nop_bytes)
        result = builtin_disassemble_wrapper(info)

        # Record some informaiton about the first 'nop' instruction we find.
        if self._nop_index is None and is_nop(result.string):
            self._nop_index = len(self._pass_1_length)
            # The offset in the following read_memory call defaults to 0.
            self._nop_bytes = info.read_memory(result.length)

        # Record information about each instruction that is disassembled.
        # This test is performed in two passes, and we need different
        # information in each pass.
        if self._first_pass:
            self._pass_1_length.append(result.length)
            self._pass_1_insn.append(result.string)
            self._pass_1_address.append(info.address)
        else:
            self._pass_2_insn.append(result.string)

        return result

    def find_replacement_candidate(self):
        """Call this after the first disassembly pass.  This identifies a suitable
        instruction to replace with 'nop' instruction(s)."""

        if self._nop_index is None:
            raise gdb.GdbError("no nop was found")

        nop_idx = self._nop_index
        nop_length = self._pass_1_length[nop_idx]

        # First we look for an instruction that is larger than a nop
        # instruction, but whose length is an exact multiple of the nop
        # instruction's length.
        replace_idx = None
        for idx in range(len(self._pass_1_length)):
            if (
                idx > 0
                and idx != nop_idx
                and not is_nop(self._pass_1_insn[idx])
                and self._pass_1_length[idx] > self._pass_1_length[nop_idx]
                and self._pass_1_length[idx] % self._pass_1_length[nop_idx] == 0
            ):
                replace_idx = idx
                break

        # If we still don't have a replacement candidate, then search again,
        # this time looking for an instruciton that is the same length as a
        # nop instruction.
        if replace_idx is None:
            for idx in range(len(self._pass_1_length)):
                if (
                    idx > 0
                    and idx != nop_idx
                    and not is_nop(self._pass_1_insn[idx])
                    and self._pass_1_length[idx] == self._pass_1_length[nop_idx]
                ):
                    replace_idx = idx
                    break

        # Weird, the nop instruction must be larger than every other
        # instruction, or all instructions are 'nop'?
        if replace_idx is None:
            raise gdb.GdbError("can't find an instruction to replace")

        # Record the instruction range that will be replaced with 'nop'
        # instructions, and mark that we are now on the second pass.
        self._start = self._pass_1_address[replace_idx]
        self._end = self._pass_1_address[replace_idx] + self._pass_1_length[replace_idx]
        self._first_pass = False
        print("Replace from 0x%x to 0x%x with NOP" % (self._start, self._end))

        # Finally, build the expected result.  Create the _check list, which
        # is a copy of _pass_1_insn, but replace the instruction we
        # identified above with a series of 'nop' instructions.
        self._check = list(self._pass_1_insn)
        nop_count = int(self._pass_1_length[replace_idx] / self._pass_1_length[nop_idx])
        nop_insn = self._pass_1_insn[nop_idx]
        nops = [nop_insn] * nop_count
        self._check[replace_idx : (replace_idx + 1)] = nops

    def check(self):
        """Call this after the second disassembler pass to validate the output."""
        if self._check != self._pass_2_insn:
            raise gdb.GdbError("mismatch")
        print("PASS")


def add_global_disassembler(dis_class):
    """Create an instance of DIS_CLASS and register it as a global disassembler."""
    dis = dis_class()
    gdb.disassembler.register_disassembler(dis, None)
    return dis


class InvalidDisassembleInfo(gdb.disassembler.DisassembleInfo):
    """An attempt to create a DisassembleInfo sub-class without calling
    the parent class init method.

    Attempts to use instances of this class should throw an error
    saying that the DisassembleInfo is not valid, despite this class
    having all of the required attributes.

    The reason why this class will never be valid is that an internal
    field (within the C++ code) can't be initialized without calling
    the parent class init method."""

    def __init__(self):
        assert current_pc is not None

    def is_valid(self):
        return True

    @property
    def address(self):
        global current_pc
        return current_pc

    @property
    def architecture(self):
        return gdb.selected_inferior().architecture()

    @property
    def progspace(self):
        return gdb.selected_inferior().progspace


# Start with all disassemblers removed.
remove_all_python_disassemblers()

print("Python script imported")
