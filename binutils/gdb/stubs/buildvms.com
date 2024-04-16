$! Command to build the gdb stub

$! Copyright (C) 2012-2024 Free Software Foundation, Inc.
$!
$! This file is part of GDB.
$!
$! This program is free software; you can redistribute it and/or modify
$! it under the terms of the GNU General Public License as published by
$! the Free Software Foundation; either version 3 of the License, or
$! (at your option) any later version.
$!
$! This program is distributed in the hope that it will be useful,
$! but WITHOUT ANY WARRANTY; without even the implied warranty of
$! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
$! GNU General Public License for more details.
$!
$! You should have received a copy of the GNU General Public License
$! along with this program.  If not, see <http://www.gnu.org/licenses/>.

$cc /debug/noopt /pointer=64 gdbstub +sys$library:sys$lib_c.tlb/lib
$ link/notraceback/sysexe/map=gdbstub.map/full/share=gdbstub.exe gdbstub,sys$inp
ut/opt
$deck
cluster=gdbzero
collect=gdbzero, XFER_PSECT
$eod
$ search /nowarnings gdbstub.map "DECC$"
$! Example of use.
$ DEFINE /nolog LIB$DEBUG sys$login:gdbstub.exe
