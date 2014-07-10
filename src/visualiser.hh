/**
 * @file
 * @author Sebastian Robitzsch <srobitzsch@gmail.com>
 *
 * @section LICENSE
 *
 * OpenMSC - MSCgen-Based Control-Plane Traffic Emulator
 *
 * Copyright (C) 2013-2014 Sebastian Robitzsch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "gnuplot-iostream.hh"
#include "dictionary.hh"

/**
 * Class to visualise EventID stream using C++ Gnuplot interface (https://code.google.com/p/gnuplot-cpp/)
 */
class Visualiser{
public:
	/**
	 * Initialise Gnuplot window (x/y range, labels, colours)
	 *
	 * @param l	copy constructor for logging functionality
	 * @return void
	 */
	void Initialise(log4cxx::LoggerPtr l, int x);
	/**
	 * Add the current map of EventIDs and their future starting times to the Gnuplot map
	 *
	 * @param map The map of EventIDs which will be sent in the future
	 */
	void UpdateEventIdMap(EVENT_MAP *map, TIME t);

	/**
	 * Update the Gnuplot plot
	 *
	 * @param currentTime Current time as a reference point to the past EventIDs and the upcoming ones
	 */
	void UpdatePlot(TIME t);

private:
	Gnuplot gnuplot;
	EVENT_MAP eventIds;
	log4cxx::LoggerPtr logger;	/** Pointer to LoggerPtr class */
	int yrangeMax;				/** Storing the maximal number of IDs getting displayed - used for 'set yrange[0:yrangeMax]' */
	int xrangeMin;				/** The window size for xrange settings in Gnuplot */
	map<EVENT_ID, int> hashMap; /** integer mapping of long long EventIDs*/
};
