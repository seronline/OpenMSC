/**
 * @file
 * @author Sebastian Robitzsch <sebastian.robitzsch@dcu.ie>
 * @version 0.1
 *
 * @section LICENSE
 *
 * OpenMSC - MSCgen-Based Control-Plane Traffic Emulator
 *
 * Copyright (C) 2013 Sebastian Robitzsch
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
#include <boost/asio.hpp>

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
bool mapLocked;
EVENT_MAP eventMap;
using boost::asio::ip::tcp;

/**
 * Generating EventIDs
 *
 * This function generates the integer numbers.
 *
 * @param pointer to Thread Identifier
 * @return voide
 */
void *generateEventIds(void *t)
{
	int seed = 1,
		time = 0,
		ctn = 0;
	timespec ts;

	base_generator_type generator(seed);
	boost::uniform_real<> uni_dist_real (0, 2);
	boost::uniform_int<> uni_dist_int (1, 10);
	boost::exponential_distribution<> exp_dist (4);
	boost::bernoulli_distribution<> bernoulli_dist (0.5);
	boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni_real (generator, uni_dist_real);
	boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (generator, uni_dist_int);
	boost::variate_generator<base_generator_type&, boost::exponential_distribution<> > exponential (generator, exp_dist);
	boost::variate_generator<base_generator_type&, boost::bernoulli_distribution<> > bernoulli (generator, bernoulli_dist);

	for (;;)
	{
		clock_gettime(CLOCK_REALTIME, &ts);

		if (time + 1 < ts.tv_sec)
		{
			cout << "EventID rate: " << ctn << " per second" << endl;
			ctn = 0;
			time = ts.tv_sec;
		}
		mut.lock();
		eventMap.insert(pair <int, int> (ts.tv_nsec + uni_int(), bernoulli()));
		EVENT_MAP_IT eventMapIt = eventMap.begin();
		mut.unlock();
		ctn++;
	}

	pthread_exit(NULL);
}

/**
 * Sending EventIDs
 *
 * This function sends the integer numbers.
 *
 * @param pointer to Thread Identifier
 * @return voide
 */
void *sendStream(void *t)
{
	EVENT_MAP eventMapCopy;
	EVENT_MAP_IT eventMapIt, eventMapCopyIt;
	timespec ts;
	string payload;
	int payloadLength;
	ostringstream convert;

	try {
		boost::asio::io_service io_service;

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "127.0.0.1", "8003");
		tcp::resolver::iterator iterator = resolver.resolve(query);

		tcp::socket socket(io_service);
		boost::asio::connect(socket, iterator);

		for(;;)
		{
			clock_gettime(CLOCK_REALTIME, &ts);

			eventMapIt = eventMap.begin();
			while (eventMapIt != eventMap.end() && eventMapIt->first < ts.tv_nsec)
			{
				mut.lock();
				convert << (*eventMapIt).second;
				payload = convert.str();
				payloadLength = payload.length();
				eventMap.erase(eventMapIt);
				mut.unlock();
				boost::asio::write(socket, boost::asio::buffer(payload, payloadLength));
				eventMapIt = eventMap.begin();
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	pthread_exit(NULL);
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

	while ((c = getopt (argc, argv, "hv")) != -1)
	{
		switch (c)
		{
		case 'v':
			std::cout << "OpenMSC - Alpha Version 0.1" << std::endl;
			return 0;
			break;
		case 'h':
			std::cout << "(c) 2013 OpenMSC\n\n";
			std::cout << "\t-v\t\tEnable debugging mode\n";
			std::cout << "\t-h\t\tPrint this help file\n";
			return 0;
			break;
		case '?':
			if (isprint (optopt))
					std::cout << "Unknown option `-%c'.\n";
			else
					std::cout << "Unknown option character `\\x%x'.\n";

			return 1;
		}
	}
	/**
	 * Reading config file using
	 * 	http://www.hyperrealm.com/libconfig/
	 * 	or a more advanced one: http://www.config4star.org
	 *
	 * 	I would prefer libconfig (available in Linux repositories)
	 */

	// Initialize and set thread joinable
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
}
