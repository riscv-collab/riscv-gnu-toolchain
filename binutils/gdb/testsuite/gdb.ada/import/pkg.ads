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

with System;
package Pkg is

   Imported_Var_Ada : Integer;
   pragma Import (C, Imported_Var_Ada, "imported_var");

   function Imported_Func_Ada return Integer;
   pragma Import (C, Imported_Func_Ada, "imported_func");

   Exported_Var_Ada : Integer := 99;
   pragma Export (C, Exported_Var_Ada, "exported_var");

   function Exported_Func_Ada return Integer;
   pragma Export (C, Exported_Func_Ada, "exported_func");

   function base return Integer;
   pragma Export (Ada, base);
   function copy return Integer;
   pragma Export (Ada, copy);

   procedure Do_Nothing (A : System.Address);

end Pkg;
