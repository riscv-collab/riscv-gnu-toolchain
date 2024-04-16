// Copyright (C) 2023-2024 Free Software Foundation, Inc.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_assignments)]


fn empty() {
}

fn main () {
    let x : u128 = 340_282_366_920_938_463_463_374_607_431_768_211_455;
    let sm : u128 = x /4;
    let y : i128 = 170_141_183_460_469_231_731_687_303_715_884_105_727;
    let mask : u128 = 0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0;

    empty();			// BREAK
}
