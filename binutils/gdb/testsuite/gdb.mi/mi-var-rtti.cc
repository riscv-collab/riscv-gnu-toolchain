/* Copyright 2012-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

struct Base {
    Base() : A(1) {}
    virtual ~Base() {}  // Enforce type to have vtable
    int A;
};

struct Derived : public Base {
    Derived() : B(2), C(3) {}
    int B;
    int C;
};


void use_rtti_for_ptr_test ()
{
  /*: BEGIN: use_rtti_for_ptr :*/
	Derived d;
	Base* ptr = &d;
	const Base* constPtr = &d;
	Base* const ptrConst = &d;
	Base const* const constPtrConst = &d;
  /*:
	set testname use_rtti_for_ptr
	set_print_object off $testname
	check_new_derived_without_rtti ptr {Base \*} $testname
	check_new_derived_without_rtti constPtr {const Base \*} $testname
	check_new_derived_without_rtti ptrConst {Base \* const} $testname
	check_new_derived_without_rtti constPtrConst {const Base \* const} \
		$testname

	set_print_object on $testname
	check_new_derived_with_rtti ptr {Derived \*} $testname
	check_new_derived_with_rtti constPtr {const Derived \*} $testname
	check_new_derived_with_rtti ptrConst {Derived \* const} $testname
	check_new_derived_with_rtti constPtrConst {const Derived \* const} \
		$testname
  :*/
	return;
  /*: END: use_rtti_for_ptr :*/
}


void use_rtti_for_ref_test ()
{
  /*: BEGIN: use_rtti_for_ref :*/
	Derived d;
	Base& ref = d;
	const Base& constRef = d;
  /*: 
	set testname use_rtti_for_ref
	set_print_object off $testname
	check_new_derived_without_rtti ref {Base \&} $testname
	check_new_derived_without_rtti constRef {const Base \&} $testname

	set_print_object on $testname
	check_new_derived_with_rtti ref {Derived \&} $testname
	check_new_derived_with_rtti constRef {const Derived \&} $testname
  :*/
	return;
  /*: END: use_rtti_for_ref :*/
}


void use_rtti_for_ptr_child_test ()
{
  /*: BEGIN: use_rtti_for_ptr_child :*/
	Derived d;
	struct S {	
		Base* ptr;
		const Base* constPtr;
		Base* const ptrConst;
		Base const* const constPtrConst;
		S ( Base* v ) :
			ptr ( v ),
			constPtr ( v ),
			ptrConst ( v ),
			constPtrConst ( v ) {}
	} s ( &d );
  /*: 
	set testname use_rtti_for_ptr_child

	set_print_object off $testname
	mi_create_varobj VAR s "create varobj for s (without RTTI) in $testname"
	mi_list_varobj_children VAR {
	    { VAR.public public 4 }
	} "list children of s (without RTTI) in $testname"
	mi_list_varobj_children VAR.public {
	    { VAR.public.ptr ptr 1 {Base \*} }
	    { VAR.public.constPtr constPtr 1 {const Base \*} }
	    { VAR.public.ptrConst ptrConst 1 {Base \* const} }
	    { VAR.public.constPtrConst constPtrConst 1 {const Base \* const} }
	} "list children of s.public (without RTTI) in $testname"
	check_derived_without_rtti VAR.public.ptr s.ptr $testname
	check_derived_without_rtti VAR.public.constPtr s.constPtr $testname
	check_derived_without_rtti VAR.public.ptrConst s.ptrConst $testname
	check_derived_without_rtti VAR.public.constPtrConst s.constPtrConst \
		$testname
	mi_delete_varobj VAR "delete varobj for s (without RTTI) in $testname"

	set_print_object on $testname
	mi_create_varobj VAR s "create varobj for s (with RTTI) in $testname"
	mi_list_varobj_children VAR {
	    { VAR.public public 4 }
	} "list children of s (with RTTI) in $testname"
	mi_list_varobj_children VAR.public {
	    { VAR.public.ptr ptr 2 {Derived \*} }
	    { VAR.public.constPtr constPtr 2 {const Derived \*} }
	    { VAR.public.ptrConst ptrConst 2 {Derived \* const} }
	    { VAR.public.constPtrConst constPtrConst 2 {const Derived \* const}}
	} "list children of s.public (with RTTI) in $testname"
	check_derived_with_rtti VAR.public.ptr s.ptr $testname
	check_derived_with_rtti VAR.public.constPtr s.constPtr $testname
	check_derived_with_rtti VAR.public.ptrConst s.ptrConst $testname
	check_derived_with_rtti VAR.public.constPtrConst s.constPtrConst \
		$testname
	mi_delete_varobj VAR "delete varobj for s (with RTTI) in $testname"
  :*/
	return;
  /*: END: use_rtti_for_ptr_child :*/
}


void use_rtti_for_ref_child_test ()
{
  /*: BEGIN: use_rtti_for_ref_child :*/
	Derived d;
	struct S {	
		Base& ref;
		const Base& constRef;
		S ( Base& v ) :
			ref ( v ),
			constRef ( v ) {}
	} s ( d );
  /*: 
	set testname use_rtti_for_ref_child

	set_print_object off $testname
	mi_create_varobj VAR s "create varobj for s (without RTTI) in $testname"
	mi_list_varobj_children VAR {
	    { VAR.public public 2 }
	} "list children of s (without RTTI) in $testname"
	mi_list_varobj_children VAR.public {
	    { VAR.public.ref ref 1 {Base \&} }
	    { VAR.public.constRef constRef 1 {const Base \&} }
	} "list children of s.public (without RTTI) in $testname"
	check_derived_without_rtti VAR.public.ref s.ref $testname
	check_derived_without_rtti VAR.public.constRef s.constRef  $testname
	mi_delete_varobj VAR "delete varobj for s (without RTTI) in $testname"

	set_print_object on $testname
	mi_create_varobj VAR s "create varobj for s (with RTTI) in $testname"
	mi_list_varobj_children VAR {
	    { VAR.public public 2 }
	} "list children of s (with RTTI) in $testname"
	mi_list_varobj_children VAR.public {
	    { VAR.public.ref ref 2 {Derived \&} }
	    { VAR.public.constRef constRef 2 {const Derived \&} }
	} "list children of s.public (with RTTI) in $testname"
	check_derived_with_rtti VAR.public.ref s.ref $testname
	check_derived_with_rtti VAR.public.constRef s.constRef $testname
	mi_delete_varobj VAR "delete varobj for s (with RTTI) in $testname"
  :*/
	return;
  /*: END: use_rtti_for_ref_child :*/
}


struct First {
    First() : F(-1) {}
    int F;
};


struct MultipleDerived : public First, Base {
    MultipleDerived() : B(2), C(3) {}
    int B;
    int C;
};


void use_rtti_with_multiple_inheritence_test ()
{
  /*: BEGIN: use_rtti_with_multiple_inheritence :*/
	MultipleDerived d;
	Base* ptr = &d;
	Base& ref = d;
  /*:
	set testname use_rtti_with_multiple_inheritence
	set_print_object off $testname
	check_new_derived_without_rtti ptr {Base \*} $testname
	check_new_derived_without_rtti ref {Base \&} $testname

	set_print_object on $testname
	mi_create_varobj_checked VAR ptr {MultipleDerived \*} \
	    "create varobj for ptr (with RTTI) in $testname"
	mi_list_varobj_children VAR {
	    { VAR.First First 1 First }
	    { VAR.Base Base 1 Base }
	    { VAR.public public 2 }
	} "list children of ptr (with RTTI) in $testname"
	mi_list_varobj_children "VAR.First" {
	    { VAR.First.public public 1 }
	} "list children of ptr.First (with RTTI) in $testname"
	mi_list_varobj_children "VAR.First.public" {
	    { VAR.First.public.F F 0 int }
	} "list children of ptr.First.public (with RTTI) in $testname"
	mi_list_varobj_children "VAR.Base" {
	    { VAR.Base.public public 1 }
	} "list children of ptr.Base (with RTTI) in $testname"
	mi_list_varobj_children "VAR.Base.public" {
	    { VAR.Base.public.A A 0 int }
	} "list children of ptr.Base.public (with RTTI) in $testname"
	mi_list_varobj_children "VAR.public" {
	    { VAR.public.B B 0 int }
	    { VAR.public.C C 0 int }
	} "list children of ptr.public (with RTTI) in $testname"

	mi_delete_varobj VAR \
	    "delete varobj for ptr (with RTTI) in $testname"
  :*/
	return;
  /*: END: use_rtti_with_multiple_inheritence :*/
}


void type_update_when_use_rtti_test ()
{
  /*: BEGIN: type_update_when_use_rtti :*/
	Base *ptr = 0;
	struct S {
		Base* ptr;
		S ( Base* v ) :
			ptr ( v ) {}
	} s ( ptr );
	Derived d;
  /*: 
	set testname type_update_when_use_rtti

	set_print_object on $testname
	mi_create_varobj_checked PTR ptr {Base \*} \
		"create varobj for ptr in $testname"
	check_derived_children_without_rtti PTR ptr $testname

	mi_create_varobj S s "create varobj for S in $testname"
	mi_list_varobj_children S {
	    { S.public public 1 }
	} "list children of s in $testname"
	mi_list_varobj_children S.public {
	    { S.public.ptr ptr 1 {Base \*} }
	} "list children of s.public in $testname"
	check_derived_children_without_rtti S.public.ptr s.ptr $testname
  :*/

	ptr = &d;
	s.ptr = &d;
  /*:
	mi_varobj_update_with_type_change PTR {Derived \*} 2 \
		"update ptr to derived in $testname"
	check_derived_with_rtti PTR ptr $testname

	mi_varobj_update_with_child_type_change S S.public.ptr {Derived \*} 2 \
		"update s.ptr to derived in $testname"
	check_derived_with_rtti S.public.ptr s.ptr $testname
  :*/

	ptr = 0;
	s.ptr = 0;
  /*:
	mi_varobj_update_with_type_change PTR {Base \*} 1 \
		"update ptr back to base type in $testname"
	mi_delete_varobj PTR "delete varobj for ptr in $testname"

	mi_varobj_update_with_child_type_change S S.public.ptr {Base \*} 1 \
		"update s.ptr back to base type in $testname"
	mi_delete_varobj S "delete varobj for s in $testname"
  :*/
	return;
  /*: END: type_update_when_use_rtti :*/
}


void skip_type_update_when_not_use_rtti_test ()
{
  /*: BEGIN: skip_type_update_when_not_use_rtti :*/
	Base *ptr = 0;
	struct S {
		Base* ptr;
		S ( Base* v ) :
			ptr ( v ) {}
	} s ( ptr );
	Derived d;
  /*: 
	set testname skip_type_update_when_not_use_rtti

	with_test_prefix "ptr is nullptr" {
	  set_print_object off $testname
	  mi_create_varobj_checked PTR ptr {Base \*} \
		"create varobj for ptr in $testname"
	  check_derived_children_without_rtti PTR ptr $testname

	  mi_create_varobj S s "create varobj for S in $testname"
	  mi_list_varobj_children S {
	      { S.public public 1 }
	  } "list children of s in $testname"
	  mi_list_varobj_children S.public {
	      { S.public.ptr ptr 1 {Base \*} }
	  } "list children of s.public in $testname"
	  check_derived_children_without_rtti S.public.ptr s.ptr $testname
	}
  :*/

	ptr = &d;
	s.ptr = &d;
  /*: 
        with_test_prefix "ptr points at d" {
	  mi_varobj_update PTR {PTR PTR.public.A} \
		"update ptr to derived type in $testname"
	  check_derived_without_rtti PTR ptr $testname

	  mi_varobj_update S {S.public.ptr S.public.ptr.public.A} \
		"update s to derived type in $testname"
	  check_derived_without_rtti S.public.ptr s.ptr $testname
	}
  :*/

	ptr = 0;
	s.ptr = 0;
  /*:
        with_test_prefix "ptr is nullptr again" {
	  mi_varobj_update PTR {PTR  PTR.public.A} \
		"update ptr back to base type in $testname"
	  mi_delete_varobj PTR "delete varobj for ptr in $testname"

	  mi_varobj_update S {S.public.ptr S.public.ptr.public.A} \
		"update s back to base type in $testname"
	  mi_delete_varobj S "delete varobj for s in $testname"
	}
  :*/
	return;
  /*: END: skip_type_update_when_not_use_rtti :*/
}


int main ()
{
	use_rtti_for_ptr_test();
	use_rtti_for_ref_test();
	use_rtti_for_ptr_child_test();
	use_rtti_for_ref_child_test();
	use_rtti_with_multiple_inheritence_test();
	type_update_when_use_rtti_test();
	skip_type_update_when_not_use_rtti_test();
	return 0;
}
