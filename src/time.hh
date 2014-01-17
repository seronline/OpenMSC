/**
 * @file time.hh
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

/**
 * \class TIME
 * This class helps to handle the different time units in OpenMSC more easily.
 */
class TIME {
  public:
	/**
	 * Constructor
	 */
	TIME();
	/**
	 * Deconstructor
	 */
	~TIME();
	/**
	 * Initialiser
	 * @param t The time
	 * @param unit The unit in which the time is given. Available types are nanosec, microsec, millisec or sec
	 */
    TIME(long double t, const char unit[]);
    /**
     * Returning the time in nano seconds
     * @return the time in nano seconds
     */
	long unsigned int nanosec() const;
	/**
	 * Returning the time in micro seconds
	 * @return the time in micro seconds
	 */
	float microsec() const;
	/**
	 * Returning the time in milli second
	 * @return the time in milli seconds
	 */
	float millisec() const;
	/**
	 * Returning the time in seconds
	 * @return the time in nano seconds
	 */
	double sec() const;
	/**
	 * Operator to use TIME class as key in std::map
	 */
	bool operator<( const TIME& other) const;
protected:
	unsigned long long time;	/** Time in nano seconds */
};
