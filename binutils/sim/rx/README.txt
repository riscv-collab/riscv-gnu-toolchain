The RX simulator offers two rx-specific configure options:

--enable-cycle-accurate  (default)
--disable-cycle-accurate

If enabled, the simulator will keep track of how many cycles each
instruction takes.  While not 100% accurate, it is very close,
including modelling fetch stalls and register latency.

--enable-sim-profile  (default)
--disable-sim-profile

If enabled, specifying "-v" twice on the simulator command line causes
the simulator to print statistics on how much time was used by each
type of opcode, and what pairs of opcodes tend to happen most
frequently, as well as how many times various pipeline stalls
happened.



The RX simulator offers many command line options:

-v - verbose output.  This prints some information about where the
program is being loaded and its starting address, as well as
information about how much memory was used and how many instructions
were executed during the run.  If specified twice, pipeline and cycle
information are added to the report.

-d - disassemble output.  Each instruction executed is printed.

-t - trace output.  Causes a *lot* of printed information about what
  every instruction is doing, from math results down to register
  changes.

--ignore-*
--warn-*
--error-*

  The RX simulator can detect certain types of memory corruption, and
  either ignore them, warn the user about them, or error and exit.
  Note that valid GCC code may trigger some of these, for example,
  writing a bitfield involves reading the existing value, which may
  not have been set yet.  The options for * are:

    null-deref - memory access to address zero.  You must modify your
      linker script to avoid putting anything at location zero, of
      course.

    unwritten-pages - attempts to read a page of memory (see below)
      before it is written.  This is much faster than the next option.

    unwritten-bytes - attempts to read individual bytes before they're
      written.

    corrupt-stack - On return from a subroutine, the memory location
      where $pc was stored is checked to see if anything other than
      $pc had been written to it most recently.

-i -w -e - these three options change the settings for all of the
  above.  For example, "-i" tells the simulator to ignore all memory
  corruption.

-E - end of options.  Any remaining options (after the program name)
  are considered to be options for the simulated program, although
  such functionality is not supported.



The RX simulator simulates a small number of peripherals, mostly in
order to provide I/O capabilities for testing and such.  The supported
peripherals, and their limitations, are documented here.

Memory

Memory for the simulator is stored in a hierarchical tree, much like
the i386's page directory and page tables.  The simulator can allocate
memory to individual pages as needed, allowing the simulated program
to act as if it had a full 4 Gb of RAM at its disposal, without
actually allocating more memory from the host operating system than
the simulated program actually uses.  Note that for each page of
memory, there's a corresponding page of memory *types* (for tracking
memory corruption).  Memory is initially filled with all zeros.

GPIO Port A

PA.DR is configured as an output-only port (regardless of PA.DDR).
When written to, a row of colored @ and * symbols are printed,
reflecting a row of eight LEDs being either on or off.

GPIO Port B

PB.DR controls the pipeline statistics.  Writing a 0 to PB.DR disables
statistics gathering.  Writing a non-0 to PB.DR resets all counters
and enables (even if already enabled) statistics gathering.  The
simulator starts with statistics enabled, so writing to PB.DR is not
needed if you want statistics on the entire program's run.

SCI4

SCI4.TDR is connected to the simulator's stdout.  Any byte written to
SCI4.TDR is written to stdout.  If the simulated program writes the
bytes 3, 3, and N in sequence, the simulator exits with an exit value
of N.

SCI4.SSR always returns "transmitter empty".


TPU1.TCNT
TPU2.TCNT

TPU1 and TPU2 are configured as a chained 32-bit counter which counts
machine cycles.  It always runs at "ICLK speed", regardless of the
clock control settings.  Writing to either of these 16-bit registers
zeros the counter, regardless of the value written.  Reading from
these registers returns the elapsed cycle count, with TPU1 holding the
most significant word and TPU2 holding the least significant word.

Note that, much like the hardware, these values may (TPU2.CNT *will*)
change between reads, so you must read TPU1.CNT, then TPU2.CNT, and
then TPU1.CNT again, and only trust the values if both reads of
TPU1.CNT were the same.
