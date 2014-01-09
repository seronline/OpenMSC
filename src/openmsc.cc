/**
 * @file
 * @author Sebastian Robitzsch <srobitzsch@gmail.com>
 * @version 0.1
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

#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <map>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "typedef.hh"
#include "msc.hh"
#include <boost/asio.hpp>
#include <libconfig.h++>

using namespace libconfig;

#if !defined(__SUNPRO_CC) || (__SUNPRO_CC > 0x530)
#include <boost/generator_iterator.hpp>
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
  using ::time;
}
#endif

boost::shared_mutex _access;
boost::condition_variable cond;
boost::mutex mut;
EVENT_MAP eventMap;
EVENT_TIMER_MAP eventTimerMap;
using boost::asio::ip::udp;
/**
 * \var seed
 * \brief seed
 *
 */
int seed = 1,
		numOfUesPerBs,
		numOfBss;
float ueCycleTime;

ReadMsc msc;
/**
 * Generating EventIDs
 *
 * This function generates a pair if TIME and EVENT_ID and adds it to the eventMap
 *
 * @param pointer to Thread Identifier
 * @return void
 */
void *generateEventIds(void *t)
{
	boost::asio::io_service io_service;
	time_t_timer timer(io_service);
	base_generator_type generator(seed);
	boost::uniform_real<> uni_dist_real (0, ueCycleTime);
	boost::uniform_int<> uni_dist_int (0, ueCycleTime);
	boost::exponential_distribution<> exp_dist (4);
	boost::bernoulli_distribution<> bernoulli_dist (0.5);
	boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni_real (generator, uni_dist_real);
	boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (generator, uni_dist_int);
	boost::variate_generator<base_generator_type&, boost::exponential_distribution<> > exponential (generator, exp_dist);
	boost::variate_generator<base_generator_type&, boost::bernoulli_distribution<> > bernoulli (generator, bernoulli_dist);

	int ueIt,
		bsIt;
	float	sTime,
			remainingTime,
			lastTime;
	EVENT_TIMER_MAP_IT eMapIt;
	timespec ts;
	EVENT_ID eventId;

	for (;;)
	{
		// Generate unique starting time for each UE between 0 and cycleTime
		for (bsIt = 1; bsIt <= numOfBss; bsIt++)
		{
			for (ueIt = 1; ueIt <= numOfUesPerBs; ueIt++)
			{
				bool newTimeFound = false;

				while (newTimeFound == false)
				{
					sTime = uni_real();

					if (eventTimerMap.find(sTime) == eventTimerMap.end())
					{
						eventTimerMap.insert(pair <float,BS_UE_PAIR> (sTime, BS_UE_PAIR (bsIt,ueIt)));
						newTimeFound = true;
					}
					//else
					//	cout << "ERROR! Time " << sTime << " already exists: " << eventTimerMap.at(sTime).first << endl;
				}
			}
		}

		eMapIt = eventTimerMap.begin();	// Initialising iterator
		lastTime = 0.0;

		// Generating EventIDs for each UE
		while (eMapIt != eventTimerMap.end())
		{
			ostringstream convert;

			timer.expires_from_now((*eMapIt).first - lastTime);
			timer.wait();
			//cout << "Generating numbers for BS ID " << (*eMapIt).second.first <<
			//		" and UE ID " << (*eMapIt).second.second << endl;
			clock_gettime(CLOCK_REALTIME, &ts);
			convert << setfill('0') << setw(5) << (*eMapIt).second.first << setw(3) << (*eMapIt).second.second;
			eventId = convert.str();
			mut.lock();
			eventMap.insert(TIME_EVENT_ID_PAIR (ts.tv_nsec, eventId));
			mut.unlock();

			if (eventTimerMap.size() == 1)
				remainingTime = ueCycleTime - (*eMapIt).first;

			lastTime = (*eMapIt).first;		// keep this time for next timer
			eventTimerMap.erase(eMapIt);
			eMapIt = eventTimerMap.begin();	// Re-initialising iterator
		}

		timer.expires_from_now(remainingTime);
		timer.wait();
	}

	pthread_exit(NULL);
}

/**
 * Sending EventIDs
 *
 * This function sends the integer numbers.
 *
 * @param pointer to Thread Identifier
 * @return void
 */
void *sendStream(void *t)
{
	EVENT_MAP eventMapCopy;
	EVENT_MAP_IT eventMapIt, eventMapCopyIt;
	timespec ts;

	try {
		boost::asio::io_service io_service;
		udp::socket s(io_service, udp::endpoint(udp::v4(), 0));
		udp::resolver resolver(io_service);
		udp::resolver::query query(udp::v4(), "127.0.0.1", "8003");
		udp::resolver::iterator iterator = resolver.resolve(query);

		for(;;)
		{
			clock_gettime(CLOCK_REALTIME, &ts);
			eventMapIt = eventMap.begin();

			while (eventMapIt != eventMap.end() && eventMapIt->first < ts.tv_nsec)
			{
				string payload;

				mut.lock();
				payload = (*eventMapIt).second;
				eventMap.erase(eventMapIt);
				mut.unlock();
				size_t payloadLength = payload.length();
				s.send_to(boost::asio::buffer(payload, payloadLength), *iterator);
				eventMapIt = eventMap.begin();
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";

		for(;;)
		{
			clock_gettime(CLOCK_REALTIME, &ts);

			eventMapIt = eventMap.begin();
			while (eventMapIt != eventMap.end() && eventMapIt->first < ts.tv_nsec)
			{
				string payload;
				int payloadLength;

				mut.lock();
				eventMapIt = eventMap.begin();
				payload = (*eventMapIt).second;
				payloadLength = payload.length();
				//cout << "Deleting " << (*eventMapIt).first << " " << (*eventMapIt).second << "\tMap Size = " << eventMap.size() << endl;
				eventMap.erase(eventMapIt);
				eventMapIt = eventMap.begin();
				mut.unlock();
			}
		}
	}

	pthread_exit(NULL);
}

/**
 * Reading configuration file
 *
 * This function leverages the libconfig library and reads the file openmsc.cfg from the doc/example directory
 *
 * @param configFileName_ Pointer of type char
 * @param ueCycleTime_ Pointer of type Time
 * @param numOfUesPerBs_ Pointer of type UE_ID
 * @param numOfBss_ Pointer of type BS_ID
 */
bool readConfiguration(char *configFileName_,
		TIME *ueCycleTime_,
		UE_ID *numOfUesPerBs_,
		BS_ID *numOfBss_)
{
	Config cfg;

	try
	{
		cfg.readFile(configFileName_);
	}
	catch(const FileIOException &fioex)
	{
		std::cerr << "I/O error while reading file." << std::endl;
		return false;
	}
	catch(const ParseException &pex)
	{
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
			<< " - " << pex.getError() << std::endl;
		return false;
	}

	const Setting& root = cfg.getRoot();

	// Output a list of all books in the inventory.
	try
	{
		cout << "Reading libconfig configuration file\n";
		const Setting &openmscConfig = root["openmscConfig"];
		openmscConfig.lookupValue("numOfUesPerBs", *numOfUesPerBs_);
		openmscConfig.lookupValue("numOfBss", *numOfBss_);
		openmscConfig.lookupValue("cycleTime", *ueCycleTime_);
	}
	catch(const SettingNotFoundException &nfex)
	{
		// Ignore.
		return(EXIT_FAILURE);
	}

	cout << "UEs per BS: " << *numOfUesPerBs_ << "\nBase Stations: " << *numOfBss_ << "\nCycle Time: " << *ueCycleTime_ << endl;

	return true;
}

/**
 * Main
 *
 * Main Function
 *
 */
int main (int argc, char** argv)
{
	long int rc;
	long int i;
	int c;
	pthread_t threads[2];
	pthread_attr_t attr;
	void *status;
	char config_file_name[] = "openmsc.cfg";

	while ((c = getopt (argc, argv, "hvs")) != -1)
	{
		switch (c)
		{
		case 'v':
			cout << "OpenMSC - Alpha Version 0.1" << endl;
			return 0;
			break;
		case 'h':
			cout << "(c) 2013 OpenMSC\n\n";
			cout << "\t-s\t\tSet seed number (not implemented yet tho)\n";
			cout << "\t-v\t\tEnable debugging mode\n";
			cout << "\t-h\t\tPrint this help file\n";
			return 0;
			break;
		case 's':
			//seed = argv+1;
			break;
		case '?':
			if (isprint (optopt))
					cout << "Unknown option `-%c'.\n";
			else
					cout << "Unknown option character `\\x%x'.\n";

			return(EXIT_FAILURE);
		}
	}

	if (!readConfiguration(config_file_name,&ueCycleTime, &numOfUesPerBs, &numOfBss))
		return(EXIT_FAILURE);

	if (msc.ReadMscConfigFile() != 0)
		return(EXIT_FAILURE);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	cout << "main() : creating generateEventIds thread" << endl;
	rc = pthread_create(&threads[0], NULL, generateEventIds, (void *)i );

	if (rc){
		cout << "Error:unable to create thread," << rc << endl;
		exit(-1);
	}

	cout << "main() : creating sendStream thread" << endl;
	rc = pthread_create(&threads[1], NULL, sendStream, (void *)i );

	if (rc){
		cout << "Error:unable to create thread," << rc << endl;
		exit(-1);
	}

	// free attribute and wait for the other threads
/*	pthread_attr_destroy(&attr);

	rc = pthread_join(threads[0], &status);

	if (rc){
		cout << "Error:unable to join," << rc << endl;
		exit(-1);
	}

	rc = pthread_join(threads[1], &status);

	if (rc){
		cout << "Error:unable to join," << rc << endl;
		exit(-1);
	}
*/
	pthread_exit(NULL);

	return(EXIT_SUCCESS);
}
