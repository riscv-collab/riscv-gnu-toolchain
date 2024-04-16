--  Copyright 2023-2024 Free Software Foundation, Inc.
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 3 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program.  If not, see <http://www.gnu.org/licenses/>.

package body Pck is
   subtype Small_Int is Integer range 0 .. 7;
   type My_Int is range -2147483648 .. 2147483647;

#if CRASHGDB = 16
    type Index is range 1 .. 16;
#end if;
#if CRASHGDB = 32
    type Index is range 1 .. 32;
#end if;
#if CRASHGDB = 64
    type Index is range 1 .. 64;
#end if;
#if CRASHGDB = 128
    type Index is range 1 .. 128;
#end if;
#if CRASHGDB = 256
    type Index is range 1 .. 256;
#end if;
#if CRASHGDB = 512
    type Index is range 1 .. 512;
#end if;
#if CRASHGDB = 1024
    type Index is range 1 .. 1024;
#end if;
#if CRASHGDB = 2048
    type Index is range 1 ..  2048;
#end if;
#if CRASHGDB = 4096
    type Index is range 1 .. 4096;
#end if;
#if CRASHGDB = 8192
    type Index is range 1 .. 8192;
#end if;
#if CRASHGDB = 16384
    type Index is range 1 .. 16384;
#end if;
#if CRASHGDB = 32768
    type Index is range 1 .. 32768;
#end if;
#if CRASHGDB = 65536
    type Index is range 1 .. 65536;
#end if;
#if CRASHGDB = 131072
    type Index is range 1 .. 131072;
#end if;
#if CRASHGDB = 262144
    type Index is range 1 .. 262144;
#end if;
#if CRASHGDB = 524288
    type Index is range 1 .. 524288;
#end if;
#if CRASHGDB = 1048576
    type Index is range 1 .. 1048576;
#end if;
#if CRASHGDB = 2097152
    type Index is range 1 .. 2097152;
#end if;

   type My_Int_Array is
     array (Index) of My_Int;
   Arr : My_Int_Array := (others => 0);

   type My_Packed_Array is array (Index) of Small_Int;
   pragma Pack (My_Packed_Array);

   Packed_Arr : My_Packed_Array := (others => 0);

   procedure Foo is
   begin
      null;
   end Foo;
end Pck;
