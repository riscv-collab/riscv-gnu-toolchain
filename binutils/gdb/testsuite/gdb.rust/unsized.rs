// Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

struct V<T: ?Sized> {
    data: T,
}

type Unsized = V<[u8]>;

fn ignore<T>(x: T) { }

fn main() {
    let v: Box<V<[u8; 3]>> = Box::new(V { data: [1, 2, 3] });
    let us: Box<Unsized> = v;
    let v2 : Box<[u8; 3]> = Box::new([1, 2, 3]);
    let us2 : Box<[u8]> = v2;

    ignore(us);     // set breakpoint here
}
