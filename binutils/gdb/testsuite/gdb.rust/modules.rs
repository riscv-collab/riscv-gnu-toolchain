// Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

fn f2() {
    println!("::f2");
}

// See https://github.com/rust-lang/rust/pull/46457
#[no_mangle]
pub static TWENTY_THREE : u16 = 23;

pub struct Generic<T>(T);

pub struct Type;

pub mod mod1 {
    pub struct Type(usize, isize);

    pub mod inner {
        pub struct Type(f64);

        pub mod innest {
            pub struct Type {pub x : u32}

            fn wrap<T> (x: T) -> ::Generic<::Generic<T>> {
                ::Generic(::Generic(x))
            }

            pub fn f1 () {
                struct Type(i8);

                let x: u8 = 0;

                let ct = ::Type;
                let ctg = wrap(ct);
                let m1t = ::mod1::Type(23, 97);
                let m1tg = wrap(m1t);
                let innert = super::Type(10101.5);
                let innertg = wrap(innert);
                let innestt = self::Type{x: 0xfff};
                let innesttg = wrap(innestt);
                let f1t = Type(9);
                let f1tg = wrap(f1t);

                let f2 = || println!("lambda f2");

                // Prevent linker from discarding symbol
                let ptr: *const u16 = &::TWENTY_THREE;

                f2();           // set breakpoint here
                f3();
                self::f2();
                super::f2();
                self::super::f2();
                self::super::super::f2();
                super::super::f2();
                ::f2();
            }

            pub fn f2() {
                println!("mod1::inner::innest::f2");
            }

            pub fn f3() {
                println!("mod1::inner::innest::f3");
            }
        }

        pub fn f2() {
            println!("mod1::inner::f2");
        }
    }

    pub fn f2() {
        println!("mod1::f2");
    }
}

fn main () {
    mod1::inner::innest::f1();
}
