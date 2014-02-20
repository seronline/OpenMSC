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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <map>
#include <argp.h>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "eventIdGenerator.hh"
#include <boost/asio.hpp>
#include <libconfig.h++>
#include <fstream>
#include <log4cxx/logger.h>
#include <log4cxx/helpers/pool.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/simplelayout.h>
#include "log4cxx/consoleappender.h"

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
using boost::asio::ip::udp;
using boost::asio::ip::tcp;
int seed = 1,
		numOfUesPerBs,
		numOfBss;
DISTRIBUTION_DEFINITION_STRUCT ueDistDef;
EVENT_MAP eventMap;
EVENT_TIMER_MAP eventTimerMap;
ReadMsc readMsc;
EventIdGenerator eventIdGenerator;
Dictionary dictionary;
IP_ADDRESS ipAddress;
PORT port;
bool TCP = false;
bool UDP = true;
bool streamToFileFlag = false;
// log4cxx
log4cxx::FileAppender * fileAppender = new log4cxx::FileAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()), "openmsc.log", false);
log4cxx::ConsoleAppender * consoleAppender = new log4cxx::ConsoleAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
log4cxx::helpers::Pool p;
log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("logger");

// ARGP
const char *argp_program_bug_address = "Sebastian Robitzsch <srobitzsch@gmail.com>";
const char *argp_program_version = "OpenMSC Version 0.2";
/* Program documentation. */
static char doc[] = "OpenMSC -- MSCgen-Based Control Plane Network Trace Emulator";
/* A description of the arguments we accept. */
static char args_doc[] = "<IP> <PORT> <DEBUG_LEVEL>";
struct arguments
{
 char *argz;
 size_t argz_len;
};
static int parse_opt (
		int key,
		char *arg,
		struct argp_state *state)
{
	switch (key)
	{
	case 'd':
	{
		if (strcmp(arg,"ERROR") == 0)
			log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getError());
		else if (strcmp(arg,"INFO") == 0)
					log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());
		else if (strcmp(arg,"DEBUG") == 0)
			log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getDebug());
		else if (strcmp(arg,"TRACE") == 0)
			log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getTrace());
		else
		{
			LOG4CXX_ERROR(logger,"No debugging level provided");
			return(EXIT_FAILURE);
		}

		LOG4CXX_INFO(logger,"Debug level set to " << arg);
		break;
	}
	case 'p':
		port = arg;
		LOG4CXX_INFO(logger,"Destination port is set to " << port);
		break;
	case 'i':
		ipAddress = arg;
		LOG4CXX_INFO(logger,"Destination IP is set to " << ipAddress);
		break;
	case 't':
		LOG4CXX_INFO(logger, "Enabling TCP communication");
		TCP = true;
		UDP = false;
		break;
	case 'f':
		LOG4CXX_INFO(logger, "Writing Stream to file");
		streamToFileFlag = true;
		break;
	case 'u':
		LOG4CXX_INFO(logger, "Enabling UDP communication");
		UDP = true;
		TCP = false;
		break;
	}

	return 0;
}
///

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
	boost::asio::deadline_timer timer(io_service);
	base_generator_type generator(seed);

	UE_ID ue;
	BS_ID bs;
	TIME remainingWaitingTime,
		sTime,
		currentTime,
		tvNsec,
		tvSec;
	EVENT_TIMER_MAP_IT eMapIt;
	timespec ts;

	for (;;)
	{
		// Generate inital starting time for each UE using the distribution specified in the openmsc.cfg file
		for (bs = 1; bs <= numOfBss; bs++)
		{
			for (ue = 1; ue <= numOfUesPerBs; ue++)
			{
				bool newTimeFound = false;	// make sure the start time is unique - OpenMSC cannot handle UEs with the exact same starting time

				while (newTimeFound == false)
				{
					clock_gettime(CLOCK_REALTIME, &ts);
					tvNsec = TIME (ts.tv_nsec, "nanosec");
					tvSec = TIME (ts.tv_sec, "sec");
					currentTime = TIME(tvSec.sec() + tvNsec.sec(), "sec");

					if (ueDistDef.distribution == UNIFORM_REAL)
					{
						boost::uniform_real<> uni_dist_real (ueDistDef.uniformMin.sec(), ueDistDef.uniformMax.sec());
						boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni_real (generator, uni_dist_real);
						sTime = TIME(uni_real(), "sec");
					}
					else if (ueDistDef.distribution == UNIFORM_INTEGER)
					{
						boost::uniform_int<> uni_dist_int (ueDistDef.uniformMin.sec(), ueDistDef.uniformMax.sec());
						boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (generator, uni_dist_int);
						sTime = TIME(uni_int(), "sec");
					}
					else if (ueDistDef.distribution == EXPONENTIAL)
					{
						boost::exponential_distribution<> exp_dist (ueDistDef.exponentialLambda);
						boost::variate_generator<base_generator_type&, boost::exponential_distribution<> > exponential (generator, exp_dist);
						sTime = TIME(exponential() + ueDistDef.exponentialMode, "sec");
					}
					// TODO implementing remaining distributions
					if (eventTimerMap.find(TIME(sTime.millisec(), "sec")) == eventTimerMap.end())
					{
						LOG4CXX_DEBUG(logger, "Inital starting time for UE " << ue
								<< " -> BS " << bs << " = " << std::setprecision(20) << (sTime.sec() + currentTime.sec()));
						eventTimerMap.insert(pair <TIME,BS_UE_PAIR> (TIME(sTime.sec() + currentTime.sec(), "sec"), BS_UE_PAIR (bs,ue)));
						newTimeFound = true;
					}
				}
			}
		}

		eMapIt = eventTimerMap.begin();

		while (eMapIt != eventTimerMap.end())
		{
			ostringstream convert;
			USE_CASE_ID useCaseId;
			useCaseId = eventIdGenerator.DetermineUseCaseId();
			eMapIt = eventTimerMap.begin();
			ue = (*eMapIt).second.second;
			bs = (*eMapIt).second.first;
			clock_gettime(CLOCK_REALTIME, &ts);
			tvNsec = TIME (ts.tv_nsec, "nanosec");
			tvSec = TIME (ts.tv_sec, "sec");
			currentTime = TIME(tvSec.sec() + tvNsec.sec(), "sec");

			if ((*eMapIt).first.sec() > currentTime.sec())
			{
				TIME tmpTime = TIME((*eMapIt).first.sec() - currentTime.sec(), "sec");
				LOG4CXX_DEBUG(logger, "Waiting " << std::setprecision(20) << tmpTime.sec() << "s");
				timer.expires_from_now(boost::posix_time::microseconds(tmpTime.microsec()));
				timer.wait();
			}

			clock_gettime(CLOCK_REALTIME, &ts);
			tvNsec = TIME(ts.tv_nsec, "nanosec");
			tvSec = TIME(ts.tv_sec, "sec");
			TIME startingTimeForThisComDescr = TIME(tvSec.sec() + tvNsec.sec(), "sec");

			for (int readMscIt = 0; readMscIt < readMsc.GetMscLength(useCaseId); readMscIt++)
			{
				EVENT_ID_VECTOR eventIdVector;
				// use-case ID, step, base-station ID, UE ID
				eventIdVector = eventIdGenerator.GetEventIdForComDescr(useCaseId, readMscIt, (*eMapIt).second.first, (*eMapIt).second.second);
				// iterate over vector (eventIdVector.size() > 1 if there was more than 1 IE in a particular primitive)
				for (unsigned int i = 0; i < eventIdVector.size(); i++)
				{
					EVENT_ID eventId;
					EVENT_MAP_IT it;
					TIME latency;
					eventId = eventIdVector.at(i);
					latency = eventIdGenerator.CalculateLatency(useCaseId, readMscIt);
					startingTimeForThisComDescr = TIME(startingTimeForThisComDescr.sec() + latency.sec(), "sec");
					mut.lock();
					it = eventMap.begin();
					// Finding spare time-slot in eventMap
					float offset = 0.0;
					while (it != eventMap.end())
					{
						it = eventMap.find(TIME(startingTimeForThisComDescr.sec() + offset, "sec"));
						offset += 0.0000000001;	//s
					}
					startingTimeForThisComDescr = TIME(startingTimeForThisComDescr.sec() + offset, "sec");
					eventMap.insert(TIME_EVENT_ID_PAIR (TIME(startingTimeForThisComDescr.sec(), "sec"), eventId));
					mut.unlock();
					LOG4CXX_TRACE (logger, "Adding EventID " << eventId
							<< " at relative time " << setprecision(20) << startingTimeForThisComDescr.sec()
							<< " to eventMap for communication descriptor " << readMscIt+1);
				}
			}
			eventTimerMap.erase(eMapIt);
			// Adding new starting time for the same UE
			bool newTimeFound = false;
			while (newTimeFound == false)
			{
				if (ueDistDef.distribution == UNIFORM_REAL)
				{
					boost::uniform_real<> uni_dist_real (ueDistDef.uniformMin.sec(), ueDistDef.uniformMax.sec());
					boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni_real (generator, uni_dist_real);
					sTime = TIME(uni_real(), "sec");
					LOG4CXX_DEBUG(logger, "Starting time distribution is UNIFORM_REAL with min = "
							<< ueDistDef.uniformMin.sec() << " and max = " << ueDistDef.uniformMax.sec()
							<< "\tValue = " << sTime.sec());
				}
				else if (ueDistDef.distribution == UNIFORM_INTEGER)
				{
					boost::uniform_int<> uni_dist_int (ueDistDef.uniformMin.sec(), ueDistDef.uniformMax.sec());
					boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (generator, uni_dist_int);
					sTime = TIME(uni_int(), "sec");
				}
				else if (ueDistDef.distribution == EXPONENTIAL)
				{
					boost::exponential_distribution<> exp_dist (ueDistDef.exponentialLambda);
					boost::variate_generator<base_generator_type&, boost::exponential_distribution<> > exponential (generator, exp_dist);
					sTime = TIME(exponential() + ueDistDef.exponentialMode, "sec");
				}
				else
				{
					LOG4CXX_ERROR(logger, "This distribution has not been implemented for calculating inter UE arrival times");
					pthread_exit(NULL);
				}
// TODO implementing remaining distributions
				clock_gettime(CLOCK_REALTIME, &ts);
				tvNsec = TIME(ts.tv_nsec, "nanosec");
				tvSec = TIME(ts.tv_sec, "sec");
				currentTime = TIME(tvSec.sec() + tvNsec.sec(), "sec");
				if (eventTimerMap.find(TIME(sTime.sec(), "sec")) == eventTimerMap.end())
				{
					TIME tmpTime;
					tmpTime = TIME(sTime.sec() + currentTime.sec(), "sec");
					LOG4CXX_DEBUG(logger, "Next starting time for UE " << ue
							<< " -> BS " << bs << " = " << std::setprecision(20) << tmpTime.sec() << "s\tCurrent time: " << currentTime.sec());
					eventTimerMap.insert(pair <TIME,BS_UE_PAIR> (tmpTime, BS_UE_PAIR (bs,ue)));
					newTimeFound = true;
				}
			}
		}
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
	ofstream file;
	boost::asio::io_service io_serviceUdp, io_serviceTcp;
	udp::socket udpSocket(io_serviceUdp, udp::endpoint(udp::v4(), 0));
	udp::resolver resolverUdp(io_serviceUdp);
	udp::resolver::query queryUdp(udp::v4(), ipAddress.c_str(), port.c_str());
	udp::resolver::iterator iteratorUdp = resolverUdp.resolve(queryUdp);
	tcp::resolver resolverTcp(io_serviceTcp);
	tcp::resolver::query queryTcp(tcp::v4(), ipAddress.c_str(), port.c_str());
	tcp::resolver::iterator iteratorTcp = resolverTcp.resolve(queryTcp);
	tcp::socket tcpSocket(io_serviceTcp);
	// Opening stream file if option was selected
	if (streamToFileFlag)
	{
		LOG4CXX_DEBUG(logger, "Opening eventStream.tsv file for writing stream to disk");
		file.open("eventStream.tsv", ios::trunc);
	}
	// Establishing TCP connection
	try
	{
		boost::asio::connect(tcpSocket, iteratorTcp);
	}
	catch (std::exception& e)
	{
		if (TCP)
			LOG4CXX_ERROR(logger, "TCP Exception: " << e.what());
	}
	LOG4CXX_DEBUG(logger, "Starting to send EventIDs");
	clock_gettime(CLOCK_REALTIME, &ts); // getting cycle starting time for absolut reference
	TIME tvNsec(ts.tv_nsec, "nanosec");
	TIME tvSec(ts.tv_sec, "sec");
	double s = tvSec.sec() + tvNsec.sec();
	TIME emulationStartTime(s, "sec");
	for(;;)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		TIME tvNsec(ts.tv_nsec, "nanosec");
		TIME tvSec(ts.tv_sec, "sec");
		double s = tvSec.sec() + tvNsec.sec();
		TIME currentTime(s, "sec");
		eventMapIt = eventMap.begin();

		while (eventMapIt != eventMap.end() && eventMapIt->first.sec() < currentTime.sec())
		{
			string payload;

			mut.lock();
			payload = (*eventMapIt).second;
			if (streamToFileFlag)
				file << setprecision(PRECISION) << currentTime.sec() - emulationStartTime.sec() << "\t" << payload << endl;
			LOG4CXX_TRACE(logger, "Sending EventID " << payload);
			eventMap.erase(eventMapIt);
			mut.unlock();
			size_t payloadLength = payload.length();

			if (UDP)
				udpSocket.send_to(boost::asio::buffer(payload, payloadLength), *iteratorUdp);
			else if (TCP)
			{
				boost::asio::write(tcpSocket, boost::asio::buffer(payload, payloadLength));
				char replyTcp[100];
				size_t reply_length = boost::asio::read(tcpSocket,
					boost::asio::buffer(replyTcp, payloadLength));
				//std::cout << "Reply is: ";
				//std::cout.write(reply, reply_length);
				//std::cout << "\n";
			}
			else
				LOG4CXX_ERROR(logger, "Neither UDP nor TCP was selected");

			eventMapIt = eventMap.begin();
		}
	}

	file.close();

	pthread_exit(NULL);
}

/**
 * Reading configuration file
 *
 * This function leverages the libconfig library and reads the file openmsc.cfg from the doc/example directory
 *
 * @param configFileName_ Pointer of type char
 * @param numOfUesPerBs_ Pointer of type UE_ID
 * @param numOfBss_ Pointer of type BS_ID
 */
bool readConfiguration(char *configFileName_,
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
		LOG4CXX_ERROR(logger,"I/O error while reading file. " << configFileName_);
		return false;
	}
	// Libconfig => 9
	/*catch(const ParseException &pex)
	{
		LOG4CXX_ERROR(logger,"Parse error at " << configFileName_ << ":" << pex.getLine() << " - " << pex.getError());
		return false;
	}*/
	catch(const ParseException &pex)
	{
		LOG4CXX_ERROR(logger,"Parse error in file " << configFileName_);
		return false;
	}

	const Setting& root = cfg.getRoot();

	// Output a list of all books in the inventory.
	try
	{
		float cT;
		const char *dist;
		LOG4CXX_INFO(logger,"Reading libconfig configuration file " << configFileName_);
		const Setting &openmscConfig = root["openmscConfig"];
		openmscConfig.lookupValue("numOfUesPerBs", *numOfUesPerBs_);
		openmscConfig.lookupValue("numOfBss", *numOfBss_);
		openmscConfig.lookupValue("ueActivity-Dist", dist);

		if (strcmp(dist,"linear") == 0)
		{
			ueDistDef.distribution = LINEAR;
		}
		else if (strcmp(dist,"exponential") == 0)
		{
			ueDistDef.distribution = EXPONENTIAL;
			openmscConfig.lookupValue("ueActivity-Dist-Lambda", ueDistDef.exponentialLambda);
			openmscConfig.lookupValue("ueActivity-Dist-Mode", ueDistDef.exponentialMode);
		}
		else if (strcmp(dist,"uniform_real") == 0)
		{
			float min, max;
			ueDistDef.distribution = UNIFORM_REAL;
			openmscConfig.lookupValue("ueActivity-Dist-Min", min);
			ueDistDef.uniformMin = TIME(min,"sec");
			openmscConfig.lookupValue("ueActivity-Dist-Max", max);
			ueDistDef.uniformMax = TIME(max,"sec");
		}
		else if (strcmp(dist,"uniform_int") == 0)
		{
			int min, max;
			ueDistDef.distribution = UNIFORM_INTEGER;
			openmscConfig.lookupValue("ueActivity-Dist-Min", min);
			ueDistDef.uniformMin = TIME(min,"sec");
			openmscConfig.lookupValue("ueActivity-Dist-Max", max);
			ueDistDef.uniformMax = TIME(max,"sec");
		}
		else if (strcmp(dist,"pareto") == 0)
		{
			ueDistDef.distribution = PARETO;
			PARETO_REAL paretoReal;
				PARETO_LOCATION paretoLocation;

		}
		else if (strcmp(dist,"gaussian") == 0)
		{
			ueDistDef.distribution = GAUSSIAN;
		}
		else
		{
			LOG4CXX_ERROR(logger,"ueActivity-Dist comprises unknown value!");
			return false;
		}

		openmscConfig.lookupValue("seed", seed);
	}
	catch(const SettingNotFoundException &nfex)
	{
		LOG4CXX_ERROR(logger, "Setting not found in openmsc.cfg");
		return false;
	}
	LOG4CXX_DEBUG(logger, "UEs per BS: " << *numOfUesPerBs_);
	LOG4CXX_DEBUG(logger, "Base Stations: " << *numOfBss_);
	readMsc.AddConfig(numOfUesPerBs_, numOfBss_);

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
	IP_ADDRESS ipAddress;
	PORT port;

	log4cxx::BasicConfigurator::configure(log4cxx::AppenderPtr(fileAppender));
	log4cxx::BasicConfigurator::configure(log4cxx::AppenderPtr(consoleAppender));
	log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());
	fileAppender->activateOptions(p);

	struct argp_option options[] =
	{
		{ "TCP", 't', 0, 0, "Using IPv4 over TCP to communicate with destination module"},
		{ "UDP", 'u', 0, 0, "Using IPv4 over UDP to communicate with destination module"},
		{ "port", 'p', "<PORT>", 0, "Port number of the receiving module"},
		{ "ip", 'i', "<IP>", 0, "IP address of the receiving module"},
		{ 0, 'f', 0, 0, "Write EventIDs to file 'eventStream.tsv'"},
		{ "debug", 'd', "<LEVEL>", 0, "Debug level (ERROR|INFO|DEBUG|TRACE)" },
		{ 0 }
	};
	struct argp argp = { options, parse_opt, args_doc, doc };

	if (argc <= 1)
	{
		LOG4CXX_ERROR(logger, "No arguments given -> ./openmsc -?\n");
		return 0;
	}

	if(argp_parse (&argp, argc, argv, 0, 0, 0) != 0)
		return(EXIT_FAILURE);

	dictionary.Init();	// initialising the dictionaries
	readMsc.InitLog(logger);
	dictionary.InitLog(logger);
	readMsc.EstablishDictConnection(&dictionary);

	if (!readConfiguration(config_file_name, &numOfUesPerBs, &numOfBss))
		return(EXIT_FAILURE);

	if (readMsc.ReadMscConfigFile() != 0)
		return(EXIT_FAILURE);

	eventIdGenerator.Init(&readMsc);
	eventIdGenerator.InitLog(logger);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	LOG4CXX_DEBUG(logger, "Creating generateEventIds thread");
	rc = pthread_create(&threads[0], NULL, generateEventIds, (void *)i );

	if (rc){
		LOG4CXX_ERROR(logger,"Unable to create generateEventIds thread, " << rc);
		exit(-1);
	}

	LOG4CXX_DEBUG(logger, "Creating sendStream thread");
	rc = pthread_create(&threads[1], NULL, sendStream, (void *)i );

	if (rc){
		LOG4CXX_ERROR(logger,"Unable to create sendStream thread, " << rc);
		exit(-1);
	}

	pthread_exit(NULL);

	return(EXIT_SUCCESS);
}
