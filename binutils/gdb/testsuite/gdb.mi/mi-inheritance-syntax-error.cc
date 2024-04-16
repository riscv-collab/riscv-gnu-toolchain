// Test for -var-info-path-expression syntax error
// caused by PR 11912
#include <string.h>
#include <stdio.h>

class A
{
	public:
		int a;
};

class C : public A
{
	public:
		C()
		{
			a = 5;
		};
		void testLocation()
		{
			z = 1;
		};
		int z;
};

int main()
{
	C c;
	c.testLocation();
	return 0;
}
