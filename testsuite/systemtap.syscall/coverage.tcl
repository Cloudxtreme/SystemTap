#!/usr/bin/env tclsh

# List of systemcalls that the systemtap.syscall tests will not cover.
#
# - bdflush: obsolete since 2.6
# - dmi_field_show: Non-syscall picked up by the "sys_*" wildcard below.
# - dmi_modalias_show: Ditto.
# - ni_syscall: non-implemented syscall
# - socketcall: common entry point for other socket syscalls
# - tux: obsolete
set badlist { bdflush dmi_field_show dmi_modalias_show ni_syscall socketcall tux }

foreach f $badlist {
    set funcname($f) -1
}

set cmd {stap -w -p2 -e "probe kernel.function(\"sys_*\").call, kernel.function(\"sys32_*\").call ? \{\}"}
if {[catch {eval exec $cmd} output]} {
    puts "ERROR running stap: $output"
    exit
}


foreach line [split $output "\n"] {
    if {[regexp {^kernel.function\(\"[Ss]y[Ss]_([^@]+)} $line match fn]} {
	if {![info exists funcname($fn)]} {
	    set funcname($fn) 0
	}
    }
    if {[regexp {^kernel.function\(\"sys32_([^@]+)} $line match fn]} {
	set fn "32_$fn"
	if {![info exists funcname($fn)]} {
	    set funcname($fn) 0
	}
    }
}

foreach filename [glob *.c] {
    if {[catch {open $filename r} fd]} {
	puts "ERROR opening $filename: $fd"
	exit
    }
    while {[gets $fd line] >= 0} {
	if {[regexp {/* COVERAGE: ([^\*]*)\*/} $line match res]} {
	    foreach f [split $res] {
		if {[info exists funcname($f)]} {
		    incr funcname($f)
		}
	    }
	}
    }
    close $fd
}

set covlist {}
set uncovlist {}
set covered 0
set uncovered 0
foreach {func val} [array get funcname] {
    if {$val > 0} {
	incr covered
	lappend covlist $func
    } elseif {$val == 0} {
	incr uncovered
	lappend uncovlist $func
    }
}

set total [expr $covered + $uncovered]
puts "Covered $covered out of $total. [format "%2.1f" [expr ($covered * 100.0)/$total]]%"

puts "\nUNCOVERED FUNCTIONS"
set i 0
foreach f [lsort $uncovlist] {
    puts -nonewline [format "%-24s" $f]
    incr i
    if {$i >= 3} {
	puts ""
	set i 0
    }
}
puts "\n"
