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

#include "visualiser.hh"
#include <sstream>
void Visualiser::Initialise(log4cxx::LoggerPtr l)
{
	logger = l;
	//gnuplot << "set xrange [-5:0]\n";

	yrangeMax = 0;
}
void Visualiser::UpdateEventIdMap(EVENT_MAP map)
{
	EVENT_MAP_IT it;

	/*LOG4CXX_INFO (logger, "Adding "
			<< map.size()
			<< " EventIDs to visualiser map which currently contains "
			<< eventIds.size()
			<< " EventIDs");
*/
	for (it = map.begin(); it != map.end(); it++)
	{
		if (eventIds.find((*it).first) == eventIds.end())
			eventIds.insert((*it));
	}
	//LOG4CXX_INFO (logger, "Visualiser map contains " << eventIds.size() << " EventIDs now")
}
void Visualiser::UpdatePlot(TIME t)
{
	EVENT_MAP_IT it;
	typedef pair <double,int> TIME_INT_PAIR;
	vector <TIME_INT_PAIR> eventIdVector, noiseVector;
	vector <TIME_INT_PAIR>::iterator eventIdVectorIterator, noiseVectorIt;

	// Deleting EventIDs which are older than 5s compared to current time 't'
	for (it = eventIds.begin(); it != eventIds.end(); it++)
	{
		if ((*it).first.sec() < (t.sec() - 6))
			eventIds.erase(it);
	}

	std::map<EVENT_ID, int>::iterator hashMapIt;
	// Hashing the EventIDs down to int
	for (it = eventIds.begin(); it != eventIds.end(); it++)
	{
		// Only get EventIDs from map which are not queued for sending
		if (!(it->first.sec() > t.sec()))
		{
			// Hash them to int
			if (hashMap.find(it->second) != hashMap.end())
			{
				hashMapIt = hashMap.find(it->second);
			}
			else
			{
				LOG4CXX_DEBUG (logger, "New hashed number for EventID "
						<< it->second << " = " << hashMap.size()+1 );
				hashMap.insert(std::pair<EVENT_ID, int>(it->second, hashMap.size()+1));
				hashMapIt = hashMap.find(it->second);
			}
			// Getting max yrange
			if (hashMapIt->second > yrangeMax)
			{
				LOG4CXX_DEBUG (logger, "yrangeMax for visualiser has increased from "
						<< yrangeMax << " to " << hashMapIt->second);
				yrangeMax = hashMapIt->second;
			}
			// Check if EventID is noise or "real" event
			int eId;
			istringstream convert(it->second);

			if (!(convert >> eId))
			{
				//cout << setprecision(20) << it->second << " is an EventID\n";
				eId = 0;
			}
			//else
				//cout << eId << " is a NoiseID\n";

			if (eId != 0)
				noiseVector.push_back(TIME_INT_PAIR (it->first.sec(),hashMapIt->second));
			else
				eventIdVector.push_back(TIME_INT_PAIR (it->first.sec(),hashMapIt->second));
		}
	}

	// Setting time-stamps to relative value 'NOW'
	for (eventIdVectorIterator = eventIdVector.begin(); eventIdVectorIterator != eventIdVector.end(); eventIdVectorIterator++)
	{
		eventIdVectorIterator->first = -1 * (t.sec() - eventIdVectorIterator->first);
		//eventIdVectorIterator->second = -1 * (eventIdVector.at(eventIdVector.size()-1).second - eventIdVectorIterator->second);
		//cout << eventIdVectorIterator->second << "\t:\t" << eventIdVectorIterator->first << endl;
	}
	for (noiseVectorIt = noiseVector.begin(); noiseVectorIt != noiseVector.end(); noiseVectorIt++)
	{
		noiseVectorIt->first = -1 * (t.sec() - noiseVectorIt->first);
		//cout << "NoiseID: " << noiseVectorIt->first << " : " << noiseVectorIt->second << endl;
	}

	gnuplot << "set yrange [0:" << yrangeMax + 10 << "]\n";
	gnuplot << "unset ytics\n";
	gnuplot << "set xtics ('Now' 0, '' -1, '' -2, '' -3, '' -4, '' -5)\n";
	gnuplot << "set xrange [-5:0]\n";
	gnuplot << "set grid xtics\n";

	// Get the rate
	int eRate, nRate;
	if (eventIdVector.size() > noiseVector.size())
	{
		if (noiseVector.size() == 0)
		{
			eRate = eventIdVector.size();
			nRate = 0;
		}
		else
		{
			eRate = eventIdVector.size() / noiseVector.size();
			nRate = 1;
		}
	}
	else if (eventIdVector.size() < noiseVector.size())
	{
		if (eventIdVector.size() == 0)
		{
			nRate = noiseVector.size();
			eRate = 0;
		}
		else
		{
			eRate = 1;
			nRate = noiseVector.size() / eventIdVector.size();
		}
	}
	else
		eRate = nRate = 1;

	if (eventIdVector.size() != 0 && noiseVector.size() != 0)
	{
		gnuplot << "set title 'EventIDs and NoiseIDs - E:N Rate = " << eRate << ":" << nRate << "' tc rgb '#c74f10'\n";
		gnuplot << "plot '-' binary" << gnuplot.binFmt1d(eventIdVector, "record") << "with points pt 7 linecolor rgb '#c74f10' noti,"
				<< "'-' binary" << gnuplot.binFmt1d(noiseVector, "record") << "with points pt 12 linecolor rgb '#002b5c' noti\n";
		gnuplot.sendBinary1d(eventIdVector);
		gnuplot.sendBinary1d(noiseVector);
	}
	else if (eventIdVector.size() == 0 && noiseVector.size() != 0)
	{
		gnuplot << "set title 'NoiseIDs only - E:N Rate = " << eRate << ":" << nRate << "' tc rgb '#c74f10'\n";
		gnuplot << "plot '-' binary" << gnuplot.binFmt1d(noiseVector, "record") << "with points pt 12 linecolor rgb '#002b5c' noti\n";
		gnuplot.sendBinary1d(noiseVector);
	}
	else if (eventIdVector.size() != 0 && noiseVector.size() == 0)
	{
		gnuplot << "set title 'EventIDs only - E:N Rate = " << eRate << ":" << nRate << "' tc rgb '#c74f10'\n";
		gnuplot << "plot '-' binary" << gnuplot.binFmt1d(eventIdVector, "record") << "with points pt 7 linecolor rgb '#c74f10' noti\n";
		gnuplot.sendBinary1d(eventIdVector);
	}

	gnuplot.flush();

}
