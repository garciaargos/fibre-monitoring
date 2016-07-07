#!/usr/bin/expect -f
#
# Access an MGSD-10080F switch and read the SFP diagnostics information
#
# Configure username and password to whatever you choose.
#
# Carlos Garcia Argos (carlos.garcia.argos@cern.ch)
# 16/6/2014

set timeout 10
set address [lindex $argv 0]
set outfile [open "temp.txt" "a"]

spawn -noecho ssh -o StrictHostKeyChecking=no username@$address
match_max 100000
expect "*?assword:*"
send -- "password\r"
expect "*/>"
set buf $expect_out(buffer)
set buf $expect_out(buffer)
foreach line [split $buf "\n"] {  # Get rid of '\r' at the beginning of lines
   set line [regsub -all "\r" $line ""]
   puts $outfile $line
}
send -- "port sfp\r"
expect "*/>"
set buf $expect_out(buffer)
foreach line [split $buf "\n"] {
   set line [regsub -all "\r" $line ""]
   puts $outfile $line
}
send -- "logout\r"
close $outfile
