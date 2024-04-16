/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#include <iostream>

using namespace std;

int dummy ()
{
  return 0;
}

class NextOverThrowDerivates
{

public:


  // Single throw an exception in this function.
  void function1 (int val)
  {
    throw val;
  }

  // Throw an exception in another function.
  void function2 (int val)
  {
    function1 (val);
  }

  // Throw an exception in another function, but handle it
  // locally.
  void function3 (int val)
  {
    {
      try
	{
	  function1 (val);
	}
      catch (...) 
	{
	  cout << "Caught and handled function1 exception" << endl;
	}
    }
  }

  void rethrow (int val)
  {
    try
      {
	function1 (val);
      }
    catch (...)
      {
	throw;
      }
  }

  void finish (int val)
  {
    // We use this to test that a "finish" here does not end up in
    // this frame, but in the one above.
    try
      {
	function1 (val);
      }
    catch (int x)
      {
      }
    function1 (val);		// marker for until
  }

  void until (int val)
  {
    function1 (val);
    function1 (val);		// until here
  }

  void resumebpt (int val)
  {
    try
      {
	throw val;
      }
    catch (int x)
      {
	dummy ();
      }
  }

};
NextOverThrowDerivates next_cases;


int
resumebpt_test (int x)
{
  try
    {
      next_cases.resumebpt (x);	    // Start: resumebpt
      next_cases.resumebpt (x + 1); // Second: resumebpt
    }
  catch (int val)
    {
      dummy ();
      x = val;
    }

  return x;
}

int main () 
{ 
  int testval = -1;

  try
    {
      next_cases.function1 (0);	// Start: first test
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: first test
    }

  try
    {
      next_cases.function2 (1);	// Start: nested throw
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: nested throw
    }

  try
    {
      // This is duplicated so we can next over one but step into
      // another.
      next_cases.function2 (2);	// Start: step in test
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: step in test
    }

  next_cases.function3 (3);	// Start: next past catch
  dummy ();
  testval = 3;			// End: next past catch

  try
    {
      next_cases.rethrow (4);	// Start: rethrow
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: rethrow
    }

  try
    {
      // Another duplicate so we can test "finish".
      next_cases.function2 (5);	// Start: first finish
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: first finish
    }

  // Another test for "finish".
  try
    {
      next_cases.finish (6);	// Start: second finish
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: second finish
    }

  // Test of "until".
  try
    {
      next_cases.finish (7);	// Start: first until
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: first until
    }

  // Test of "until" with an argument.
  try
    {
      next_cases.until (8);	// Start: second until
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: second until
    }

  // Test of "advance".
  try
    {
      next_cases.until (9);	// Start: advance
    }
  catch (int val)
    {
      dummy ();
      testval = val;		// End: advance
    }

  // Test of "resumebpt".
  testval = resumebpt_test (10);

  testval = 32;			// done
}
