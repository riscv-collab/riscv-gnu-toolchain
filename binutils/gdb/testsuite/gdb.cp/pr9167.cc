#include <iostream>

template<typename DATA>
struct ATB
{
    int data;
    ATB() : data(0) {}
};


template<typename DATA,
	 typename DerivedType >
class A : public ATB<DATA>
{
public:
    static DerivedType const DEFAULT_INSTANCE;
};

template<typename DATA, typename DerivedType>
const DerivedType A<DATA, DerivedType>::DEFAULT_INSTANCE;

class B : public A<int, B>
{
    
};

int main()
{
    B b;
    // If this if-block is removed then GDB shall
    // not infinitely recurse when trying to print b.

    return 0;		// marker
}


