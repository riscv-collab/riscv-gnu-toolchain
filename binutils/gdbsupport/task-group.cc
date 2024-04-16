/* Task group

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "task-group.h"
#include "thread-pool.h"

namespace gdb
{

class task_group::impl : public std::enable_shared_from_this<task_group::impl>
{
public:

  explicit impl (std::function<void ()> &&done)
    : m_done (std::move (done))
  { }
  DISABLE_COPY_AND_ASSIGN (impl);

  ~impl ()
  {
    if (m_started)
      m_done ();
  }

  /* Add a task to the task group.  */
  void add_task (std::function<void ()> &&task)
  {
    m_tasks.push_back (std::move (task));
  };

  /* Start this task group.  */
  void start ();

  /* True if started.  */
  bool m_started = false;
  /* The tasks.  */
  std::vector<std::function<void ()>> m_tasks;
  /* The 'done' function.  */
  std::function<void ()> m_done;
};

void
task_group::impl::start ()
{
  std::shared_ptr<impl> shared_this = shared_from_this ();
  m_started = true;
  for (size_t i = 0; i < m_tasks.size (); ++i)
    {
      gdb::thread_pool::g_thread_pool->post_task ([=] ()
      {
	/* Be sure to capture a shared reference here.  */
	shared_this->m_tasks[i] ();
      });
    }
}

task_group::task_group (std::function<void ()> &&done)
  : m_task (new impl (std::move (done)))
{
}

void
task_group::add_task (std::function<void ()> &&task)
{
  gdb_assert (m_task != nullptr);
  m_task->add_task (std::move (task));
}

void
task_group::start ()
{
  gdb_assert (m_task != nullptr);
  m_task->start ();
  m_task.reset ();
}

} /* namespace gdb */
