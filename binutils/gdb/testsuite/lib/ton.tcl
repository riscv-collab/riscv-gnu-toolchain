# This was imported into gdb from:
# https://github.com/jorge-leon/ton

# This software is copyrighted by Georg Lehner <jorge@at.anteris.net>.
# The following terms apply to all files associated with the software
# unless explicitly disclaimed in individual files.

# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose,
# provided that existing copyright notices are retained in all copies
# and that this notice is included verbatim in any distributions. No
# written agreement, license, or royalty fee is required for any of the
# authorized uses.  Modifications to this software may be copyrighted by
# their authors and need not follow the licensing terms described here,
# provided that the new terms are clearly indicated on the first page of
# each file where they apply.

# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
# FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
# DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
# NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
# THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
# MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

# GOVERNMENT USE: If you are acquiring this software on behalf of the
# U.S. government, the Government shall have only "Restricted Rights" in
# the software and related documentation as defined in the Federal
# Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
# are acquiring the software on behalf of the Department of Defense, the
# software shall be classified as "Commercial Computer Software" and the
# Government shall have only "Restricted Rights" as defined in Clause
# 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
# authors grant the U.S. Government and others acting in its behalf
# permission to use and distribute the software in accordance with the
# terms specified in this license.


# leg20180331: ton / TON - Tcl Object Notation
#
# This package provides manipulation functionality for TON - a data
# serialization format with a direct mapping to JSON.
#
# In its essence, a JSON parser is provided, which can convert a JSON
# string into a Tcllib json style dictionary (dicts and arrays mixed),
# into a jimhttp style dictionary (only dicts) or into a nested, typed
# Tcl list.
#
# Finally, TON can be converted into (unformatted) JSON.

namespace eval ton {
    namespace export json2ton

    variable version 0.4
    
}
proc ton::json2ton json {
    # Parse JSON string json
    #
    # return: TON
    
    set i [trr $json [string length $json]]
    if {!$i} {return ""}
    lassign [jscan $json $i] i ton
    if {[set i [trr $json $i]]} {
	error "json string invalid:[incr i -1]: left over characters."
    }
    return $ton
}
proc ton::trr {s i} {
    # Trim righthand whitespace on the first i characters of s.
    # return: number of remaining characters in s
    
    while {[set j $i] &&
	   ([string is space [set c [string index $s [incr i -1]]]]
	    || $c eq "\n")} {}
    return $j
}
proc ton::jscan {json i {d :}} {
    # Scan JSON in first i characters of string json.
    # d is the default delimiter list for the next token.
    #
    # return list of
    # - remaining characters in json
    # - TON
    #
    # The string must already be whitespace trimmed from the right.

    incr i -1

    if {[set c [string index $json $i]] eq "\""} {
	str $json [incr i -1]
    } elseif {$c eq "\}"} {
	obj $json $i
    } elseif {$c eq "\]"} {
	arr $json $i
    } elseif {$c in {e l}} {
	lit $json $i
    } elseif {[string match {[0-9.]} $c]} {
	num $json $i $c $d
    } else {
	error "json string end invalid:$i: ..[string range $json $i-10 $i].."
    }
}
proc ton::num {json i c d} {
    # Parse number from position i in string json to the left.
    # c .. character at position i
    # d .. delimiter on which to stop
    #
    # return list:
    # - remaining string length
    # - TON of number
    
    set float [expr {$c eq "."}]
    for {set j $i} {$i} {incr i -1} {
	if {[string match $d [set c [string index $json $i-1]]]} break
	set float [expr {$float || [string match "\[eE.]" $c]}]
    }
    set num [string trimleft [string range $json $i $j]]
    if {!$float && [string is entier $num]} {
	    list $i "i $num"
    } elseif {$float && [string is double $num]} {
	list $i "d $num"
    } else {
	error "number invalid:$i: $num."
    }
}
proc ton::lit {json i} {
    # Parse literal from position i in string json to the left
    # return list:
    # - remaining string length
    # - TON of literal

    if {[set c [string index $json $i-1]] eq "u"} {
	list [incr i -3] "l true"
    } elseif {$c eq "s"} {
	list [incr i -4] "l false"
    } elseif {$c eq "l"} {
	list [incr i -3] "l null"
    } else {
	set e [string range $json $i-3 $i]
	error "literal invalid:[incr i -1]: ..$e."
    }
}
proc ton::str {json i} {
    # Parse string from position i in string json to the left
    # return list:
    # - remaining string length
    # - TON of string
    
    for {set j $i} {$i} {incr i -1} {
	set i [string last \" $json $i]
	if {[string index $json $i-1] ne "\\"} break
    }
    if {$i==-1} {
	error "json string start invalid:$i: exhausted while parsing string."
    }
    list $i "s [list [string range $json $i+1 $j]]"
}
proc ton::arr {json i} {
    # Parse array from i characters in string json
    # return list:
    # - remaining string length
    # - TON of array
    
    set i [trr $json $i]
    if {!$i} {
	error "json string invalid:0: exhausted while parsing array."
    }
    if {[string index $json $i-1] eq "\["} {
	return [list [incr i -1] a]
    }
    set r {}
    while {$i} {
	lassign [jscan $json $i "\[,\[]"] i v
	lappend r \[$v\]
	set i [trr $json $i]
	incr i -1
	if {[set c [string index $json $i]] eq ","} {
	    set i [trr $json $i]
	    continue
	} elseif {$c eq "\["} break
	error "json string invalid:$i: parsing array."
    }
    lappend r a
    return [list $i [join [lreverse $r]]]
}
proc ton::obj {json i} {
    # Parse array from i character in string json
    # return list:
    # - remaining string length
    # - TON of object

    set i [trr $json $i]
    if {!$i} {
    	error "json string invalid:0: exhausted while parsing object."
    }
    if {[string index $json $i-1] eq "\{"} {
	return [list [incr i -1] o]
    }
    set r {}
    while {$i} {
	lassign [jscan $json $i] i v
	set i [trr $json $i]
	incr i -1
	if {[string index $json $i] ne ":"} {
	    error "json string invalid:$i: parsing key in object."
	}
	set i [trr $json $i]
	lassign [jscan $json $i] i k
	lassign $k type k
	if {$type ne "s"} {
	    error "json string invalid:[incr i -1]: key not a string."
	}
	lappend r \[$v\] [list $k]
	set i [trr $json $i]
	incr i -1
	if {[set c [string index $json $i]] eq ","} {
	    set i [trr $json $i]
	    continue
	} elseif {$c eq "\{"} break
	error "json string invalid:$i: parsing object."	
    }
    lappend r o
    return [list $i [join [lreverse $r]]]
}
# TON decoders
namespace eval ton::2list {
    proc atom {type v} {list $type $v}
    foreach type {i d s l} {
	interp alias {} $type {} [namespace current]::atom $type
    }
    proc a args {
	set r a
	foreach v $args {lappend r $v}
	return $r
    }
    proc o args {
	set r o
	foreach {k v} $args {lappend r $k $v}
	return $r
    }
    # There is plenty of room for validation in get
    # array index bounds
    # object key existence
    proc get {l args} {
	foreach k $args {
	    switch [lindex $l 0] {
		o {set l [dict get [lrange $l 1 end] $k]}
		a {set l [lindex $l [incr k]]}
		default {
		    error "error: key $k to long, or wrong data: [lindex $l 0]"
		}
	    }
	}
	return $l
    }
}
namespace eval ton::2dict {
    proc atom v {return $v}
    foreach type {i d l s} {
	interp alias {} $type {} [namespace current]::atom
    }
    proc a args {return $args}
    proc o args {return $args}
}
namespace eval ton::a2dict {
    proc atom v {return $v}
    foreach type {i d l s} {
	interp alias {} $type {} [namespace current]::atom
    }
    proc a args {
	set i -1
	set r {}
	foreach v $args {
	    lappend r [incr i] $v
	}
	return $r
    }
    proc o args {return $args}
}
namespace eval ton::2json {
    proc atom v {return $v}
    foreach type {i d l} {
	interp alias {} $type {} [namespace current]::atom
    }
    proc a args {
	return "\[[join $args {, }]]"
    }
    proc o args {
	set r {}
	foreach {k v} $args {lappend r "\"$k\": $v"}
	return "{[join $r {, }]}"
    }
    proc s s {return "\"$s\""}
}

package provide ton $ton::version
