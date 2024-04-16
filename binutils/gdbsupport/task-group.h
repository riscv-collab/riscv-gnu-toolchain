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

#ifndef GDBSUPPORT_TASK_GROUP_H
#define GDBSUPPORT_TASK_GROUP_H

#include <memory>

namespace gdb
{

/* A task group is a collection of tasks.  Each task in the group is
   submitted to the thread pool.  When all the tasks in the group have
   finished, a final action is run.  */

class task_group
{
public:

  explicit task_group (std::function<void ()> &&done);
  DISABLE_COPY_AND_ASSIGN (task_group);

  /* Add a task to the task group.  All tasks must be added before the
     group is started.  Note that a task may not throw an
     exception.  */
  void add_task (std::function<void ()> &&task);

  /* Start this task group.  A task group may only be started once.
     This will submit all the tasks to the global thread pool.  */
  void start ();

private:

  class impl;

  /* A task group is just a facade around an impl.  This is done
     because the impl object must live as long as its longest-lived
     task, so it is heap-allocated and destroyed when the last task
     completes.  */
  std::shared_ptr<impl> m_task;
};

} /* namespace gdb */

#endif /* GDBSUPPORT_TASK_GROUP_H */
