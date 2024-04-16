# Copyright (C) 2015-2024 Free Software Foundation, Inc.

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
from gdb.unwinder import Unwinder, FrameId


# These are set to test whether invalid register names cause an error.
add_saved_register_errors = {}
read_register_error = False


class TestUnwinder(Unwinder):
    AMD64_RBP = 6
    AMD64_RSP = 7
    AMD64_RIP = None

    def __init__(self):
        Unwinder.__init__(self, "test unwinder")
        self.char_ptr_t = gdb.lookup_type("unsigned char").pointer()
        self.char_ptr_ptr_t = self.char_ptr_t.pointer()
        self._last_arch = None

    # Update the register descriptor AMD64_RIP based on ARCH.
    def _update_register_descriptors(self, arch):
        if self._last_arch != arch:
            TestUnwinder.AMD64_RIP = arch.registers().find("rip")
            self._last_arch = arch

    def _read_word(self, address):
        return address.cast(self.char_ptr_ptr_t).dereference()

    def __call__(self, pending_frame):
        """Test unwinder written in Python.

        This unwinder can unwind the frames that have been deliberately
        corrupted in a specific way (functions in the accompanying
        py-unwind.c file do that.)
        This code is only on AMD64.
        On AMD64 $RBP points to the innermost frame (unless the code
        was compiled with -fomit-frame-pointer), which contains the
        address of the previous frame at offset 0. The functions
        deliberately corrupt their frames as follows:
                     Before                 After
                   Corruption:           Corruption:
                +--------------+       +--------------+
        RBP-8   |              |       | Previous RBP |
                +--------------+       +--------------+
        RBP     + Previous RBP |       |    RBP       |
                +--------------+       +--------------+
        RBP+8   | Return RIP   |       | Return  RIP  |
                +--------------+       +--------------+
        Old SP  |              |       |              |

        This unwinder recognizes the corrupt frames by checking that
        *RBP == RBP, and restores previous RBP from the word above it.
        """

        # Check that we can access the architecture of the pending
        # frame, and that this is the same architecture as for the
        # currently selected inferior.
        inf_arch = gdb.selected_inferior().architecture()
        frame_arch = pending_frame.architecture()
        if inf_arch != frame_arch:
            raise gdb.GdbError("architecture mismatch")

        self._update_register_descriptors(frame_arch)

        try:
            # NOTE: the registers in Unwinder API can be referenced
            # either by name or by number. The code below uses both
            # to achieve more coverage.
            bp = pending_frame.read_register("rbp").cast(self.char_ptr_t)
            if self._read_word(bp) != bp:
                return None
            # Found the frame that the test program has corrupted for us.
            # The correct BP for the outer frame has been saved one word
            # above, previous IP and SP are at the expected places.
            previous_bp = self._read_word(bp - 8)
            previous_ip = self._read_word(bp + 8)
            previous_sp = bp + 16

            try:
                pending_frame.read_register("nosuchregister")
            except ValueError as ve:
                global read_register_error
                read_register_error = str(ve)

            frame_id = FrameId(
                pending_frame.read_register(register=TestUnwinder.AMD64_RSP),
                pending_frame.read_register(TestUnwinder.AMD64_RIP),
            )
            unwind_info = pending_frame.create_unwind_info(frame_id)
            unwind_info.add_saved_register(TestUnwinder.AMD64_RBP, previous_bp)
            unwind_info.add_saved_register(value=previous_ip, register="rip")
            unwind_info.add_saved_register(register="rsp", value=previous_sp)

            global add_saved_register_errors
            try:
                unwind_info.add_saved_register("nosuchregister", previous_sp)
            except ValueError as ve:
                add_saved_register_errors["unknown_name"] = str(ve)

            try:
                unwind_info.add_saved_register(999, previous_sp)
            except ValueError as ve:
                add_saved_register_errors["unknown_number"] = str(ve)

            try:
                unwind_info.add_saved_register("rsp", 1234)
            except TypeError as ve:
                add_saved_register_errors["bad_value"] = str(ve)

            return unwind_info
        except (gdb.error, RuntimeError):
            return None


global_test_unwinder = TestUnwinder()
gdb.unwinder.register_unwinder(None, global_test_unwinder, True)

# These are filled in by the simple_unwinder class.
captured_pending_frame = None
captured_pending_frame_repr = None
captured_unwind_info = None
captured_unwind_info_repr = None


class simple_unwinder(Unwinder):
    def __init__(self, name, sp=0x123, pc=0x456):
        super().__init__(name)
        self._sp = sp
        self._pc = pc

    def __call__(self, pending_frame):
        global captured_pending_frame
        global captured_pending_frame_repr
        global captured_unwind_info
        global captured_unwind_info_repr

        assert pending_frame.is_valid()

        if captured_pending_frame is None:
            captured_pending_frame = pending_frame
            captured_pending_frame_repr = repr(pending_frame)
            fid = FrameId(self._sp, self._pc)
            uw = pending_frame.create_unwind_info(frame_id=fid)
            uw.add_saved_register("rip", gdb.Value(0x123))
            uw.add_saved_register("rbp", gdb.Value(0x456))
            uw.add_saved_register("rsp", gdb.Value(0x789))
            captured_unwind_info = uw
            captured_unwind_info_repr = repr(uw)
        return None


# Return a dictionary of information about FRAME.
def capture_frame_information(frame):
    name = frame.name()
    level = frame.level()
    language = frame.language()
    function = frame.function()
    architecture = frame.architecture()
    pc = frame.pc()
    sal = frame.find_sal()
    try:
        block = frame.block()
        assert isinstance(block, gdb.Block)
    except RuntimeError as rte:
        assert str(rte) == "Cannot locate block for frame."
        block = "RuntimeError: " + str(rte)

    return {
        "name": name,
        "level": level,
        "language": language,
        "function": function,
        "architecture": architecture,
        "pc": pc,
        "sal": sal,
        "block": block,
    }


# List of information about each frame.  The index into this list is
# the frame level.  This is populated by
# capture_all_frame_information.
all_frame_information = []


# Fill in the global ALL_FRAME_INFORMATION list.
def capture_all_frame_information():
    global all_frame_information

    all_frame_information = []

    gdb.newest_frame().select()
    frame = gdb.selected_frame()
    count = 0

    while frame is not None:
        frame.select()
        info = capture_frame_information(frame)
        level = info["level"]
        info["matched"] = False

        while len(all_frame_information) <= level:
            all_frame_information.append(None)

        assert all_frame_information[level] is None
        all_frame_information[level] = info

        if frame.name == "main" or count > 10:
            break

        count += 1
        frame = frame.older()


# Assert that every entry in the global ALL_FRAME_INFORMATION list was
# matched by the validating_unwinder.
def check_all_frame_information_matched():
    global all_frame_information
    for entry in all_frame_information:
        assert entry["matched"]


# An unwinder that doesn't match any frames.  What it does do is
# lookup information from the PendingFrame object and compare it
# against information stored in the global ALL_FRAME_INFORMATION list.
class validating_unwinder(Unwinder):
    def __init__(self):
        super().__init__("validating_unwinder")

    def __call__(self, pending_frame):
        info = capture_frame_information(pending_frame)
        level = info["level"]

        global all_frame_information
        old_info = all_frame_information[level]

        assert old_info is not None
        assert not old_info["matched"]

        for key, value in info.items():
            assert key in old_info, key + " not in old_info"
            assert type(value) == type(old_info[key])
            if isinstance(value, gdb.Block):
                assert value.start == old_info[key].start
                assert value.end == old_info[key].end
                assert value.is_static == old_info[key].is_static
                assert value.is_global == old_info[key].is_global
            else:
                assert str(value) == str(old_info[key])

        old_info["matched"] = True
        return None


print("Python script imported")
