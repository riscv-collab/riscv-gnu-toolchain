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


pub struct HiBob {
    pub field1: i32,
    field2: u64,
}

struct ByeBob(i32, u64);

enum Something {
    One,
    Two,
    Three
}

enum MoreComplicated {
    One,
    Two(i32),
    Three(HiBob),
    Four{this: bool, is: u8, a: char, struct_: u64, variant: u32},
}

// tests the nonzero optimization, but fields are reversed
enum NonZeroOptimized {
    Empty,
    Value(String),
}

fn diff2(x: i32, y: i32) -> i32 {
    x - y
}

// Empty function, should not have "void"
// or "()" in its return type
fn empty() {

}

pub struct Unit;

// This triggers the non-zero optimization that yields a different
// enum representation in the debug info.
enum SpaceSaver {
    Thebox(u8, Box<i32>),
    Nothing,
}

enum Univariant {
    Foo {a: u8}
}
enum UnivariantAnon {
    Foo(u8)
}

enum ParametrizedEnum<T> {
    Val { val: T },
    Empty,
}

struct ParametrizedStruct<T> {
    next: ParametrizedEnum<Box<ParametrizedStruct<T>>>,
    value: T
}

struct StringAtOffset {
    pub field1: &'static str,
    pub field2: i32,
    pub field3: &'static str,
}

// A simple structure whose layout won't be changed by the compiler,
// so that ptype/o testing will work on any platform.
struct SimpleLayout {
    f1: u16,
    f2: u16
}

enum EmptyEnum {}

#[derive(Debug)]
struct EnumWithNonzeroOffset {
    a: Option<u8>,
    b: Option<u8>,
}

fn main () {
    let a = ();
    let b : [i32; 0] = [];

    let mut c = 27;
    let d = c = 99;

    let e = MoreComplicated::Two(73);
    let e2 = MoreComplicated::Four {this: true, is: 8, a: 'm',
                                    struct_: 100, variant: 10};

    let f = "hi bob";
    let g = b"hi bob";
    let h = b'9';

    let fslice = &f[3..];

    let i = ["whatever"; 8];

    let j = Unit;
    let j2 = Unit{};

    let k = SpaceSaver::Nothing;
    let l = SpaceSaver::Thebox(9, Box::new(1729));

    let v = Something::Three;
    let w = [1,2,3,4];
    let w_ptr = &w[0];
    let x = (23, 25.5);
    let y = HiBob {field1: 7, field2: 8};
    let z = ByeBob(7, 8);

    let field1 = 77;
    let field2 = 88;

    let univariant = Univariant::Foo {a : 1};
    let univariant_anon = UnivariantAnon::Foo(1);

    let slice = &w[2..3];
    let fromslice = slice[0];
    let slice2 = &slice[0..1];

    let all1 = &w[..];
    let all2 = &slice[..];

    let from1 = &w[1..];
    let from2 = &slice[1..];

    let to1 = &w[..3];
    let to2 = &slice[..1];

    let st = StringAtOffset { field1: "hello", field2: 1, field3: "world" };

    // tests for enum optimizations

    let str_some = Some("hi".to_string());
    let str_none = None::<String>;
    let box_some = Some(Box::new(1u8));
    let box_none = None::<Box<u8>>;
    let int_some = Some(1u8);
    let int_none = None::<u8>;
    let custom_some = NonZeroOptimized::Value("hi".into());
    let custom_none = NonZeroOptimized::Empty;

    let parametrized = ParametrizedStruct {
        next: ParametrizedEnum::Val {
            val: Box::new(ParametrizedStruct {
                next: ParametrizedEnum::Empty,
                value: 1,
            })
        },
        value: 0,
    };

    let simplelayout = SimpleLayout { f1: 8, f2: 9 };

    let empty_enum_value: EmptyEnum;

    let nonzero_offset = EnumWithNonzeroOffset { a: Some(1), b: None };

    println!("{}, {}", x.0, x.1);        // set breakpoint here
    println!("{}", diff2(92, 45));
    empty();
}
