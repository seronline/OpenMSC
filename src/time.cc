/**
 * @file time.cc
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

#include "time.hh"
#include <string.h>
#include <iostream>
#include <iomanip>
TIME::TIME()
{
	time = 0;
}
TIME::~TIME() { }
TIME::TIME(long double t, const char unit[]) {

	if (strcmp(unit, "sec") == 0)
	{
		time = (unsigned long long)(t * 1000000000);
	}
	else if (strcmp(unit, "millisec") == 0)
	{
		time = (unsigned long long)(t * 1000000);
	}
	else if (strcmp(unit, "microsec") == 0)
	{
		time = (unsigned long long)(t * 1000);
	}
	else if (strcmp(unit, "nanosec") == 0)
	{
		time = (unsigned long long)t;
	}
	else
	{
		std::cout << "ERROR!!! Unknown time unit " << unit << std::endl;
		time = 0.0;
	}
}
long unsigned int TIME::nanosec() const
{
	return time;
}
float TIME::microsec() const
{
	return (float)time / 1000;
}
float TIME::millisec() const
{
	return (float)time / 1000000;
}
double TIME::sec() const
{
	return (double)time / 1000000000;
}
bool TIME::operator<( const TIME& other) const
{
	return time < other.time;
}
