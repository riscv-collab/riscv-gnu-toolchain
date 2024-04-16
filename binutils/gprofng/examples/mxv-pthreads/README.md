# README for the matrix-vector multiplication demo code

## Synopsis

This program implements the multiplication of a matrix and a vector.  It is
written in C and has been parallelized using the Pthreads parallel programming
model.  Each thread gets assigned a contiguous set of rows of the matrix to
work on and the results are stored in the output vector.

The code initializes the data, executes the matrix-vector multiplication, and
checks the correctness of the results. In case of an error, a message to this
extent is printed and the program aborts. Otherwise it prints a one line
message on the screen.

## About this code

This is a standalone code, not a library. It is meant as a simple example to
experiment with gprofng.

## Directory structure

There are four directories:

1. `bindir` - after the build, it contains the executable.

2. `experiments` - after the installation, it contains the executable and
also has an example profiling script called `profile.sh`.

3. `objects` - after the build, it contains the object files.

4. `src` - contains the source code and the make file to build, install,
and check correct functioning of the executable.

## Code internals

This is the main execution flow:

* Parse the user options.
* Compute the internal settings for the algorithm.
* Initialize the data and compute the reference results needed for the correctness
check.
* Create and execute the threads. Each thread performs the matrix-vector
multiplication on a pre-determined set of rows.
* Verify the results are correct.
* Print statistics and release the allocated memory.

## Installation

The Makefile in the `src` subdirectory can be used to build, install and check the
code.

Use `make` at the command line to (re)build the executable called `mxv-pthreads`. It will be
stored in the directory `bindir`:

```
$ make
gcc -o ../objects/main.o -c -g -O -Wall -Werror=undef -Wstrict-prototypes main.c
gcc -o ../objects/manage_data.o -c -g -O -Wall -Werror=undef -Wstrict-prototypes manage_data.c
gcc -o ../objects/workload.o -c -g -O -Wall -Werror=undef -Wstrict-prototypes workload.c
gcc -o ../objects/mxv.o -c -g -O -Wall -Werror=undef -Wstrict-prototypes mxv.c
gcc -o ../bindir/mxv-pthreads  ../objects/main.o ../objects/manage_data.o ../objects/workload.o ../objects/mxv.o -lm -lpthread
ldd ../bindir/mxv-pthreads
	linux-vdso.so.1 (0x0000ffff9ea8b000)
	libm.so.6 => /lib64/libm.so.6 (0x0000ffff9e9ad000)
	libc.so.6 => /lib64/libc.so.6 (0x0000ffff9e7ff000)
	/lib/ld-linux-aarch64.so.1 (0x0000ffff9ea4e000)
$
```
The `make install` command installs the executable in directory `experiments`.

```
$ make install
Installed mxv-pthreads in ../experiments
$
```
The `make check` command may be used to verify the program works as expected:

```
$ make check
Running mxv-pthreads in ../experiments
mxv: error check passed - rows = 1000 columns = 1500 threads = 2
$
```
The `make clean` comand removes the object files from the `objects` directory
and the executable from the `bindir` directory.

The `make veryclean` command implies `make clean`, but also removes the
executable from directory `experiments`.

## Usage

The code takes several options, but all have a default value. If the code is
executed without any options, these defaults will be used.  To get an overview of
all the options supported, and the defaults, use the `-h` option:

```
$ ./mxv-pthreads -h
Usage: ./mxv-pthreads [-m <number of rows>] [-n <number of columns] [-r <repeat count>] [-t <number of threads] [-v] [-h]
	-m - number of rows, default = 2000
	-n - number of columns, default = 3000
	-r - the number of times the algorithm is repeatedly executed, default = 200
	-t - the number of threads used, default = 1
	-v - enable verbose mode, off by default
	-h - print this usage overview and exit
$
```

For more extensive run time diagnostic messages use the `-v` option.

As an example, these are the options to compute the product of a 2000x1000 matrix
with a vector of length 1000 and use 4 threads.  Verbose mode has been enabled:

```
$ ./mxv-pthreads -m 2000 -n 1000 -t 4 -v
Verbose mode enabled
Allocated data structures
Initialized matrix and vectors
Defined workload distribution
Assigned work to threads
Thread 0 has been created
Thread 1 has been created
Thread 2 has been created
Thread 3 has been created
Matrix vector multiplication has completed
Verify correctness of result
Error check passed
mxv: error check passed - rows = 2000 columns = 1000 threads = 4
$
```

## Executing the examples

Directory `experiments` contains the `profile.sh` script.  This script
checks if gprofng can be found and for the executable to be installed.

The script will then run a data collection experiment, followed by a series
of invocations of `gprofng display text` to show various views.  The results
are printed on stdout.

To include the commands executed in the output of the script, and store the
results in a file called `LOG`, execute the script as follows:

```
$ bash -x ./profile.sh >& LOG
```

## Additional comments

* The reason that compiler based inlining is disabled is to make the call tree
look more interesting.  For the same reason, the core multiplication function
`mxv_core` has inlining disabled through the `void __attribute__ ((noinline))`
attribute. Of course you're free to change this. It certainly does not affect
the workings of the code.

* This distribution includes a script called `profile.sh`. It is in the
`experiments` directory and meant as an example for (new) users of gprofng.
It can be used to produce profiles at the command line. It is also suitable
as a starting point to develop your own profiling script(s).
