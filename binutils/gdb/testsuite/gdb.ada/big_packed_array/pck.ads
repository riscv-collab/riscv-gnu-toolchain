--  Copyright 2019-2024 Free Software Foundation, Inc.
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

package Pck is
   type Enum_Idx is
      (e_000, e_001, e_002, e_003, e_004, e_005, e_006, e_007, e_008,
       e_009, e_010, e_011, e_012, e_013, e_014, e_015, e_016, e_017,
       e_018, e_019, e_020, e_021, e_022, e_023, e_024, e_025, e_026,
       e_027, e_028, e_029, e_030, e_031, e_032, e_033, e_034, e_035,
       e_036, e_037, e_038, e_039, e_040, e_041, e_042, e_043, e_044,
       e_045, e_046, e_047, e_048, e_049, e_050, e_051, e_052, e_053,
       e_054, e_055, e_056, e_057, e_058, e_059, e_060, e_061, e_062,
       e_063, e_064, e_065, e_066, e_067, e_068, e_069, e_070, e_071,
       e_072, e_073, e_074, e_075, e_076, e_077, e_078, e_079, e_080,
       e_081, e_082, e_083, e_084, e_085, e_086, e_087, e_088, e_089,
       e_090, e_091, e_092, e_093, e_094, e_095, e_096, e_097, e_098,
       e_099, e_100, e_101, e_102, e_103, e_104, e_105, e_106, e_107,
       e_108, e_109, e_110, e_111, e_112, e_113, e_114, e_115, e_116,
       e_117, e_118, e_119, e_120, e_121, e_122, e_123, e_124, e_125,
       e_126, e_127, e_128, e_129, e_130, e_131, e_132, e_133, e_134,
       e_135, e_136, e_137, e_138, e_139, e_140, e_141, e_142, e_143,
       e_144, e_145, e_146, e_147, e_148, e_149, e_150, e_151, e_152,
       e_153, e_154, e_155, e_156, e_157, e_158, e_159, e_160, e_161,
       e_162, e_163, e_164, e_165, e_166, e_167, e_168, e_169, e_170,
       e_171, e_172, e_173, e_174, e_175, e_176, e_177, e_178, e_179,
       e_180, e_181, e_182, e_183, e_184, e_185, e_186, e_187, e_188,
       e_189, e_190, e_191, e_192, e_193, e_194, e_195);

   type PA is array (Enum_Idx) of Boolean;
   pragma Pack (PA);

   type T is array (Enum_Idx) of Boolean;
   pragma Pack (T);
   T_Empty : constant T := (others => False);

   type Bad_Packed_Table is new T;

   Procedure Do_Nothing (A : System.Address);
end Pck;
