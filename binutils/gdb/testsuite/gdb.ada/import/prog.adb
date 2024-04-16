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

with Pkg;

procedure Prog is
   Ordinary_Var : Integer := 78;

   Local_Imported_Var : Integer;
   pragma Import (C, Local_Imported_Var, "imported_var");

   function Local_Imported_Func return Integer;
   pragma Import (C, Local_Imported_Func, "imported_func");
begin
   Local_Imported_Var := Local_Imported_Func;  --  BREAK
   Pkg.Imported_Var_Ada := Pkg.Imported_Func_Ada;
   Pkg.Do_Nothing (Pkg.Imported_Func_Ada'Address);
   Pkg.Do_Nothing (Pkg.Exported_Func_Ada'Address);
end Prog;
