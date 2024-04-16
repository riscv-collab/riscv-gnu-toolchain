/* BPF simulator support code header
   Copyright (C) 2023-2024 Free Software Foundation, Inc.

   Contributed by Oracle Inc.

   This file is part of GDB, the GNU debugger.

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

#ifndef BPF_SIM_H
#define BPF_SIM_H

/* The following struct determines the state of the simulator.  */

struct bpf_sim_state
{

};

#define BPF_SIM_STATE(sd) ((struct bpf_sim_state *) STATE_ARCH_DATA (sd))

#endif /* ! BPF_SIM_H */
