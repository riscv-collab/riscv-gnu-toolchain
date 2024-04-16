/* Python DAP interpreter

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
#include "python-internal.h"
#include "interps.h"
#include "cli-out.h"
#include "ui.h"

class dap_interp final : public interp
{
public:

  explicit dap_interp (const char *name)
    : interp (name),
      m_ui_out (new cli_ui_out (gdb_stdout))
  {
  }

  ~dap_interp () override = default;

  void init (bool top_level) override;

  void suspend () override
  {
  }

  void resume () override
  {
  }

  void exec (const char *command) override
  {
    /* Just ignore it.  */
  }

  void set_logging (ui_file_up logfile, bool logging_redirect,
		    bool debug_redirect) override
  {
    /* Just ignore it.  */
  }

  ui_out *interp_ui_out () override
  {
    return m_ui_out.get ();
  }

private:

  std::unique_ptr<ui_out> m_ui_out;
};

void
dap_interp::init (bool top_level)
{
  gdbpy_enter enter_py;

  gdbpy_ref<> dap_module (PyImport_ImportModule ("gdb.dap"));
  if (dap_module == nullptr)
    gdbpy_handle_exception ();

  gdbpy_ref<> func (PyObject_GetAttrString (dap_module.get (), "run"));
  if (func == nullptr)
    gdbpy_handle_exception ();

  gdbpy_ref<> result_obj (PyObject_CallObject (func.get (), nullptr));
  if (result_obj == nullptr)
    gdbpy_handle_exception ();

  current_ui->input_fd = -1;
  current_ui->m_input_interactive_p = false;
}

void _initialize_py_interp ();
void
_initialize_py_interp ()
{
  /* The dap code uses module typing, available starting python 3.5.  */
#if PY_VERSION_HEX >= 0x03050000
  interp_factory_register ("dap", [] (const char *name) -> interp *
    {
      return new dap_interp (name);
    });
#endif
}
