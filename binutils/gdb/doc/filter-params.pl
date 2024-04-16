#!/usr/bin/perl

# This Perl script tweaks GDB sources to be more useful for Doxygen.

while (<>) {
    # Allow "/* * " as an equivalent to "/** ", better for Emacs compat.
    s/\/\* \* /\/** /sg;
    # Manually expand macro seen in structs and such.
    s/ENUM_BITFIELD[ \t]*\((.*?)\)/__extension__ enum $1/sg;
    print;
}
