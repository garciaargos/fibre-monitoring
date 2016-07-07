#!/usr/bin/awk -f
#
# Process data in the dump of the expect script
# that accesses the SFP diagnostics in the Planet
# Switch MGSD-10080F
#
# Carlos Garcia Argos (carlos.garcia.argos@cern.ch)
# 16/6/2014

BEGIN {
   FS=" "
   write=0
   firstline=1
   OFS="\t"
   thistime=strftime("%Y.%m.%d %H:%M.00")  # Actual date and time
   daytime=strftime("%b %d %H:%M:00")      # Human-friendly date
   hostname=""
}
{
   if (hostname=="") {   # Get the hostname from a prompt line
      if ($0~/>/) {
         last=match($1,":")
         hostname=tolower(substr($1,1,last-1))
      }
   }

   if ((write==0) && (NF>=6)) {  # Find the header of the SFP diagnostics info
      if ($1 ~ /Temperature/) {
         write=1
      }
   } else if (write==1) {
      if ((firstline==0) && ($2!="--")) { # Get the second line of the port info and output
         if (strtonum($1)<0) {
      	    print "Error"
      	 } else {
            print daytime,hostname":"port,thistime,$1,$2,$3,$4,$5 
         }
         firstline=1
      } else {
         if ((firstline==1) && ($1 ~ /[0-9]/) && ($2!="--")) { # Get the first line of the port info
            port=strtonum($1)
            firstline=0
         }
      }
      if ($1=="10") { # When we get to the last port, start over
         write=0
         hostname=""
         firstline=1
      }
   }
}
