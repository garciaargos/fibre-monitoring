#!/bin/bash
#
# Script to run a Expect script to access the switches and read the SFPs 
# diagnostics chips, then an AWK script to process the data and save it
# in a nice format
# 
# Carlos Garcia Argos (carlos.garcia.argos@cern.ch)
# 16/6/2014
#
# Changelog:
#  16/06/2014: first version, working fine with 1 switch
#  30/06/2014: second switch in, sometimes one of the two spits negative numbers
#              in all SFP columns. Fixed by repeating the read-out until it 
#              stops doing it.
#  01/07/2014: fixed infinite loop problem spotted overnight. Maximum of 20 tries.
#    


nswitches=3	# change this, not the array

cd /home/pi/SwitchReading

# IP addresses of the switches (or names)
switch[0]='192.168.1.1'
switch[1]='192.168.1.2'
switch[2]='192.168.1.3'
switch[3]='192.168.1.4'
switch[4]='192.168.1.5'
switch[5]='192.168.1.6'
switch[6]='192.168.1.7'
switch[7]='192.168.1.8'

for((i=0;i<$nswitches;i++)) do
	thisisfine=0
	count=0
	while [ $count -le 20 ]
	do
		./switchread.sh ${switch[$i]} > /dev/null
		./ProcessData.sh temp.txt > SwitchLog.txt
		# If something's fishy, repeat until it stops being wrong or 20 tries
		if grep -v Error SwitchLog.txt
		then 
			thisisfine=1
			count=999
		else
			_now=$(date +"%Y%m%d_%H%M%S")
			cp temp.txt "$_now"
			count=$(( $count + 1 ))
		fi
		rm temp.txt
	done
	if [ $thisisfine -gt 0 ]
	then
		cat SwitchLog.txt >> MergedLog.txt
	fi
	rm SwitchLog.txt
done

# Generate png files using ROOT
root -b -q "PlotFibreMonSwitch.C(-1)"

