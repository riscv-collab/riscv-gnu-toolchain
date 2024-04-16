// Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#![allow(warnings)]

fn five() -> i32 { 5 }

fn main() {
    let foo = Foo {x: 5, f: five};
    foo.print();  // set breakpoint here
    println!("Hello, world! {}, {}, {}", foo.f(), (foo.f)(),
             foo.g ());
}

struct Foo {
    x :i32,
    f: fn () -> i32,
}

impl Foo {
    fn print(&self) {
        println!("hello {}", self.x)
    }

    fn f(&self) -> i32 { 6 }
    fn g(&self) -> i32 { 7 }
}
