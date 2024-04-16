/* Self tests for frame_info_ptr.

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"

#include "frame.h"
#include "gdbsupport/selftest.h"
#include "scoped-mock-context.h"
#include "test-target.h"

namespace selftests {

static void
validate_user_created_frame (frame_id id)
{
  SELF_CHECK (id.stack_status == FID_STACK_VALID);
  SELF_CHECK (id.stack_addr == 0x1234);
  SELF_CHECK (id.code_addr_p);
  SELF_CHECK (id.code_addr == 0x5678);
}

static frame_info_ptr
user_created_frame_callee (frame_info_ptr frame)
{
  validate_user_created_frame (get_frame_id (frame));

  reinit_frame_cache ();

  validate_user_created_frame (get_frame_id (frame));

  return frame;
}

static void
test_user_created_frame ()
{
  scoped_mock_context<test_target_ops> mock_context
    (current_inferior ()->arch ());
  frame_info_ptr frame = create_new_frame (0x1234, 0x5678);

  validate_user_created_frame (get_frame_id (frame));

  /* Pass the frame to a callee, which calls reinit_frame_cache.  This lets us
     validate that the reinflation in both the callee and caller restore the
     same frame_info object.  */
  frame_info_ptr callees_frame_info = user_created_frame_callee (frame);

  validate_user_created_frame (get_frame_id (frame));
  SELF_CHECK (frame.get () == callees_frame_info.get ());
}

} /* namespace selftests */

void _initialize_frame_info_ptr_selftests ();
void
_initialize_frame_info_ptr_selftests ()
{
  selftests::register_test ("frame_info_ptr_user",
			    selftests::test_user_created_frame);
}
