#!/usr/bin/awk -f
#
# @author Sebastian Robitzsch <srobitzsch@gmail.com>
#
# @section LICENSE
#
# OpenMSC - MSCgen-Based Control-Plane Traffic Emulator
#
# Copyright (C) 2013-2014 Sebastian Robitzsch
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Run the script using the following command: ./plotStreamOverTime.awk patterns.csv eventStream.tsv 
# Note, the patterns.csv file MUST be provided first
# This script does NOT work if csOverlap has been set to false in openmsc.cfg

function hashId(id)
{
	it = 1
	id2String = ""id
	while (it <= hashedIds[0])
	{
		if (hashedIds[it] == id2String)
			return it
		it++
	}
	hashedIds[0]++
	hashedIds[hashedIds[0]] = id2String
	print hashedIds[hashedIds[0]] "\t" hashedIds[0] >> HASHED_FILE
	return hashedIds[0]
}
BEGIN {
	PID = 1	# The Pattern ID which should be coloured from patterns.csv
	NID = 1	# The Noise ID which should be coloured
	rgbColourPattern1 = "#c74f10"	# OpenMSC colour
	rgbColourPattern2 = "#C7AA10"	# adjacent colour to OpenMSC colour
	rgbColourNoise = "#80ADFF"
	# Don't touch!
	numOfIds = 0
	ORDINARY_STREAM_FILE="eventStreamOrdinary"
	COLOURED_PATTERN_STREAM_FILE="eventStreamPatternColoured"
	COLOURED_PATTERN_STREAM_START_FILE="eventStreamPatternStartColoured"
	COLOURED_NOISE_STREAM_FILE="eventStreamNoiseColoured"
	HASHED_FILE="hashedEventIds"
	system("rm " ORDINARY_STREAM_FILE " " COLOURED_PATTERN_STREAM_FILE " " COLOURED_NOISE_STREAM_FILE " "HASHED_FILE " " COLOURED_PATTERN_STREAM_START_FILE)
	hashedIds[0] = 0
	lastTime = 0
	patterOccurrences = 0
	sumTimeDiff = 0
	PM_ID = 1
}
{
	if (FILENAME == "patterns.csv")
	{
		if ((FNR-1) == PID)
		{
			print "Reading "$0
			split($2, tmp, ",")
			i = 1
			while (length(tmp[i]) > 0)
			{
				patternModelToBeColoured[i] = tmp[i]
				patternModelToBeColoured[0] = i
				i++
			}
			print "Length: " patternModelToBeColoured[0]
		}
		
	}
	else if (FILENAME == "eventStream.tsv")# && $1 >= 10.0 && $1 <= 200.0)
	{
		PID_FOUND=0
		# Check if the wrong Pattern Model start was found
		if ($2 == patternModelToBeColoured[1] && PM_ID != 1)
		{
			print "Wrong Pattern ID was found at t = " $1
			print "Expected "patternModelToBeColoured[PM_ID] " but got 1st of PID "PID" ("$2")"

			# Write all wrongly extracted IDs to ordinary stream
			for (arrayIt = 1; arrayIt <= PM_ID; arrayIt++)
				print colouredPatternModel[arrayIt,0] "\t" hashId(colouredPatternModel[arrayIt,1]) >> ORDINARY_STREAM_FILE

			delete colouredPatternModel
			PM_ID = 1
			colouredPatternModel[PM_ID,0] = $1	# Storing time stamp
			colouredPatternModel[PM_ID,1] = $2	# Storing Pattern ID
			PM_ID++
		}
		else if ($2 == patternModelToBeColoured[PM_ID] && PM_ID != patternModelToBeColoured[0])
		{
			PID_FOUND=1
			if (PM_ID == 1)
				print "Pattern Model start found at time " $1 "\t-> " $2
			colouredPatternModel[PM_ID,0] = $1	# Storing time stamp
			colouredPatternModel[PM_ID,1] = $2	# Storing Pattern ID
			hashId($2)
			PM_ID++
		}
		# Last Pattern ID of PM detected
		else if ($2 == patternModelToBeColoured[PM_ID] && PM_ID == patternModelToBeColoured[0])
		{
			PID_FOUND=1
			print "End of Pattern Model found at time " $1 "\t -> " $2
			colouredPatternModel[PM_ID,0] = $1	# Storing time stamp
			colouredPatternModel[PM_ID,1] = $2	# Storing Pattern ID
			hashId($2)
			print colouredPatternModel[1,0] "\t" hashId(colouredPatternModel[1,1]) >> COLOURED_PATTERN_STREAM_START_FILE
			print colouredPatternModel[1,0] "\t" hashId(colouredPatternModel[1,1]) >>COLOURED_PATTERN_STREAM_FILE

			for (arrayIt = 2; arrayIt <= PM_ID; arrayIt++)
			{
				print colouredPatternModel[arrayIt,0] "\t" hashId(colouredPatternModel[arrayIt,1]) >> COLOURED_PATTERN_STREAM_FILE
			}
			print "" >> COLOURED_PATTERN_STREAM_FILE
			delete colouredPatternModel
			PM_ID = 1 # reset the variable
		}
	
		if (PID_FOUND == 0)
		{
			if ($2 == NID)
				print $1 "\t" hashId($2) >> COLOURED_NOISE_STREAM_FILE	
			else
				print $1 "\t" hashId($2) >> ORDINARY_STREAM_FILE
		}
		numOfIds++
	}
}
END {
	GNUPLOT="gnuplot.plt"
	print "set xlabel 'Time [s]'" > GNUPLOT
	print "set title 'Total Number of IDs in this Window = " numOfIds "'" >> GNUPLOT
	print "set ylabel 'Identifiers'" >> GNUPLOT
	print "unset ytics" >> GNUPLOT
	print "set grid" >> GNUPLOT
	print "set terminal eps enhanced" >> GNUPLOT
	print "set output 'streamOverTime_PID-"PID"_NID-"NID".eps'" >> GNUPLOT
	print "set key top left" >> GNUPLOT
	print "plot '"ORDINARY_STREAM_FILE"' using 1:2 with points pt 7 ps 0.1 lc rgb '#555555' noti, \\" >> GNUPLOT
	print "'" COLOURED_PATTERN_STREAM_FILE "' using 1:2 with line lw 2 lc rgb '"rgbColourPattern1"' noti, \\" >> GNUPLOT
	print "'" COLOURED_PATTERN_STREAM_START_FILE "' using 1:2 with points pt 7 ps 1 lc rgb '"rgbColourPattern1"' noti, \\" >> GNUPLOT
	print "'" COLOURED_PATTERN_STREAM_FILE "' using 1:2 with points pt 7 ps 0.3 lc rgb '"rgbColourPattern2"' title 'Pattern ID " PID "', \\" >> GNUPLOT
	print "'" COLOURED_NOISE_STREAM_FILE "' using 1:2 with points pt 13 ps 0.5 lc rgb '"rgbColourNoise"' title 'Noise ID "NID"'" >> GNUPLOT
	system("gnuplot " GNUPLOT)
	system("rm " GNUPLOT)
	print "Figure 'streamOverTime_PID-"PID"_NID-"NID".eps' created"
}
