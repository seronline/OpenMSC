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
#include <boost/random/normal_distribution.hpp>
#include <boost/random/gamma_distribution.hpp>
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
#include <limits>

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
boost::mutex mut, visualiserMapMutex;
using boost::asio::ip::udp;
using boost::asio::ip::tcp;
int seed = 1,
		numOfUesPerBs,
		numOfBss,
		visualiserWindowSize,	/** The size of the window of the visualiser in seconds*/
		visualiserUpdateInterval = 0; /** The update interval of the visualiser in milliseconds. Default 0 (continuous plotting)*/
unsigned int stopRate;	/** Number indicating after how many EvenIDs OpenMSC should stop sending and automatically ends*/
DISTRIBUTION_DEFINITION_STRUCT ueDistDef;
NOISE_DESCRIPTION_STRUCT noiseDescrStruct;
EVENT_MAP eventMap, visualiserMap;
EVENT_TIMER_MAP eventTimerMap;
HASHED_NOISE_EVENT_ID_MAP hashedNoiseEventIdMap;
ReadMsc readMsc;
EventIdGenerator eventIdGenerator;
Dictionary dictionary;
IP_ADDRESS ipAddress;
PORT port;
bool TCP = false,
UDP = true,
streamToFileFlag = false,
PRINT_EVENT_ID_RATE = false,
AUTOMATICALLY_STOP_SENDING = false,
VISUALISER=false;
const int MAX_INT = std::numeric_limits<int>::max();
// log4cxx
log4cxx::FileAppender * fileAppender = new log4cxx::FileAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()), "openmsc.log", false);
log4cxx::ConsoleAppender * consoleAppender = new log4cxx::ConsoleAppender(log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
log4cxx::helpers::Pool p;
log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("logger");

// ARGP
const char *argp_program_bug_address = "Sebastian Robitzsch <srobitzsch@gmail.com>";
const char *argp_program_version = "OpenMSC Version 0.3";

/* Program documentation. */
static char doc[] = "OpenMSC -- MSCgen-Based Control Plane Network Trace Emulator";
/* A description of the arguments we accept. */
static char args_doc[] = "<IP> <PORT>";
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
	case 'r':
		PRINT_EVENT_ID_RATE=true;
		break;
	case 's':
		AUTOMATICALLY_STOP_SENDING = true;
		stopRate = atoi(arg);
		LOG4CXX_INFO(logger,"Stopping OpenMSC when " << stopRate << " were sent");
		break;
	case 'v':
		VISUALISER=true;
		visualiserWindowSize = atoi(arg);
		LOG4CXX_INFO(logger,"Enable visualiser with window size = " << visualiserWindowSize);
		break;
	case 'w':
		visualiserUpdateInterval = atoi(arg);
		LOG4CXX_INFO(logger, "Update interval for visualiser set to = " << visualiserUpdateInterval);
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
	base_generator_type generator(seed),
			generatorComDescriptor(seed),
			generatorUseCase(seed);
	UE_ID ue;
	BS_ID bs;
	TIME remainingWaitingTime,
		sTime,
		currentTime,
		tvNsec,
		tvSec;
	EVENT_TIMER_MAP_IT eMapIt;
	timespec ts;

	eventIdGenerator.WritePatterns2File();

	for (;;)
	{
		// Generate inital starting time for each UE using the distribution specified in the openmsc.cfg file
		for (bs = 1; bs <= numOfBss; bs++)
		{
			for (ue = 1; ue <= numOfUesPerBs; ue++)
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
					LOG4CXX_TRACE(logger, "Starting time distribution is UNIFORM_REAL with min = "
											<< ueDistDef.uniformMin.sec() << " and max = " << ueDistDef.uniformMax.sec()
											<< "\tValue = " << sTime.sec());
				}
				else if (ueDistDef.distribution == UNIFORM_INTEGER)
				{
					boost::uniform_int<> uni_dist_int ((int)ueDistDef.uniformMin.sec(), (int)ueDistDef.uniformMax.sec());
					boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (generator, uni_dist_int);
					sTime = TIME(uni_int(), "sec");
					LOG4CXX_TRACE(logger, "Starting time distribution is UNIFORM_INT with Min = "
											<< ueDistDef.uniformMin.sec() << " and Max = " << ueDistDef.uniformMax.sec()
											<< "\tValue = " << sTime.sec());
				}
				else if (ueDistDef.distribution == EXPONENTIAL)
				{
					boost::exponential_distribution<> exp_dist (ueDistDef.exponentialLambda);
					boost::variate_generator<base_generator_type&, boost::exponential_distribution<> > exponential (generator, exp_dist);
					sTime = TIME(exponential(), "sec");
					LOG4CXX_TRACE(logger, "Starting time distribution is EXPONENTIAL with Lambda = "
											<< ueDistDef.exponentialLambda
											<< "\tValue = " << sTime.sec());
				}
				else if (ueDistDef.distribution == GAUSSIAN)
				{
					boost::normal_distribution<> gau_dist (ueDistDef.gaussianMu, ueDistDef.gaussianSigma);
					boost::variate_generator<base_generator_type&, boost::normal_distribution<> > gaussian (generator, gau_dist);
					sTime = TIME(gaussian(), "sec");
					LOG4CXX_TRACE(logger, "Starting time distribution is GAUSSIAN with Mu = " << ueDistDef.gaussianMu
											<< " and Sigma = " << ueDistDef.gaussianSigma
											<< "\tValue = " << sTime.sec());
				}
				else if (ueDistDef.distribution == GAMMA)
				{
					boost::gamma_distribution<> gamma_dist (ueDistDef.gammaAlpha, ueDistDef.gammaBeta);
					boost::variate_generator<base_generator_type&, boost::gamma_distribution<> > gamma (generator, gamma_dist);
					sTime = TIME(gamma(), "sec");
					LOG4CXX_TRACE(logger, "Starting time distribution is GAMMA with Alpha = " << ueDistDef.gammaAlpha
											<< " and Beta = " << ueDistDef.gammaBeta
											<< "\tValue = " << sTime.sec());
				}
				else if (ueDistDef.distribution == ERLANG)
				{
					boost::gamma_distribution<> erlang_dist ((int)ueDistDef.erlangAlpha, (int)ueDistDef.erlangBeta);
					boost::variate_generator<base_generator_type&, boost::gamma_distribution<> > erlang (generator, erlang_dist);
					sTime = TIME(erlang(), "sec");
					LOG4CXX_TRACE(logger, "Starting time distribution is GAMMA with Alpha = " << (int)ueDistDef.gammaAlpha
											<< " and Beta = " << (int)ueDistDef.gammaBeta
											<< "\tValue = " << sTime.sec());
				}
				else
				{
					LOG4CXX_ERROR(logger, "This distribution has not been implemented to calculate UE arrival times");
					pthread_exit(NULL);
				}
				bool newTimeFound = false;	// make sure the start time is unique - OpenMSC cannot handle UEs with the exact same starting time

				while (newTimeFound == false)
				{
					if (eventTimerMap.find(TIME((currentTime.sec() + sTime.sec()), "sec")) == eventTimerMap.end())
					{
						LOG4CXX_DEBUG(logger, "Initial starting time for UE " << ue
								<< " -> BS " << bs << " = " << std::setprecision(20) << (sTime.sec() + currentTime.sec())
								<< " using distribution " << ueDistDef.distribution);
						eventTimerMap.insert(pair <TIME,BS_UE_PAIR> (TIME(sTime.sec() + currentTime.sec(), "sec"), BS_UE_PAIR (bs,ue)));
						newTimeFound = true;
					}
					else
					{
						TIME sTmp = TIME(1, "millisec");
						sTime = TIME(sTime.sec() + sTmp.sec(), "sec");
					}
				}
			}
		}

		eMapIt = eventTimerMap.begin();

		while (eMapIt != eventTimerMap.end())
		{
			ostringstream convert;
			USE_CASE_ID useCaseId;
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
				LOG4CXX_TRACE(logger, "Waiting " << std::setprecision(20) << tmpTime.sec()
						<< "s before generating another communication description");
				timer.expires_from_now(boost::posix_time::microseconds(tmpTime.microsec()));
				timer.wait();
			}

			clock_gettime(CLOCK_REALTIME, &ts);
			tvNsec = TIME(ts.tv_nsec, "nanosec");
			tvSec = TIME(ts.tv_sec, "sec");
			TIME startingTimeForThisComDescr = TIME(tvSec.sec() + tvNsec.sec(), "sec");
			EVENT_ID_VECTOR eventIdVectorPeriodic;
			useCaseId = eventIdGenerator.DetermineUseCaseId(&generatorUseCase);
			LOG4CXX_DEBUG(logger, "Use-Case ID for UE " << (*eMapIt).second.first << " - BS " << (*eMapIt).second.second << " = " << useCaseId);
			for (int readMscIt = 0; readMscIt < readMsc.GetMscLength(useCaseId); readMscIt++)
			{
				EVENT_ID_VECTOR eventIdVector;
				// use-case ID, step, base-station ID, UE ID
				eventIdVector = eventIdGenerator.GetEventIdForComDescr(useCaseId, readMscIt,
						(*eMapIt).second.first, (*eMapIt).second.second);
				TIME latency;
				TIME offset = TIME(0.0, "sec");
				latency = eventIdGenerator.CalculateLatency(useCaseId, readMscIt, &generatorComDescriptor);
				// iterate over vector (eventIdVector.size() > 1 if there was more than 1 IE in a particular primitive)
				for (unsigned int i = 0; i < eventIdVector.size(); i++)
				{
					EVENT_ID eventId;
					EVENT_MAP_IT it;
					eventId = eventIdVector.at(i);
					//Periodic communication descriptor - store it for next msc step, if this is NOT the last step
					if (readMsc.GetPeriodicCommunicationDescriptorFlag(useCaseId,readMscIt))
					{
						eventIdVectorPeriodic = eventIdVector;
					}
					else
					{
						if (eventIdVectorPeriodic.size() > 0)
						{
							TIME latencyPeriodic,
							periodicStartTime = TIME(startingTimeForThisComDescr.sec(), "sec");
							// Generate as many periodic events as time is until the next '=>' communication descriptor
							while ((periodicStartTime.sec() + offset.sec()) < (startingTimeForThisComDescr.sec() + latency.sec()))
							{
								latencyPeriodic = eventIdGenerator.CalculateLatency(useCaseId, readMscIt-1,
										&generatorComDescriptor);
								periodicStartTime = TIME(periodicStartTime.sec() + latencyPeriodic.sec(), "sec");
								for (int eventIdVectorPeriodicIt = 0; eventIdVectorPeriodicIt < eventIdVectorPeriodic.size(); eventIdVectorPeriodicIt++)
								{
									mut.lock();
									it = eventMap.begin();
									// Finding spare time-slot in eventMap
									while (it != eventMap.end())
									{
										it = eventMap.find(TIME(periodicStartTime.sec() + offset.sec(), "sec"));
										if (it != eventMap.end())
											offset = TIME(offset.millisec() + 1, "millisec");
									}
									// just make sure that the new time is still smaller than the starting time for the next comm descriptor
									if ((periodicStartTime.sec() + offset.sec())
											< (startingTimeForThisComDescr.sec() + latency.sec()))
									{
										eventMap.insert(TIME_EVENT_ID_PAIR (TIME(periodicStartTime.sec() + offset.sec(), "sec"),
												eventIdVectorPeriodic.at(eventIdVectorPeriodicIt)));
										LOG4CXX_TRACE (logger, "Adding periodic EventID "
												<< eventIdVectorPeriodic.at(eventIdVectorPeriodicIt)
												<< " at relative time " << setprecision(20) << periodicStartTime.sec() + offset.sec()
												<< " to eventMap for use-case " << useCaseId << " and communication descriptor " << readMscIt-1);
									}
									mut.unlock();
									if (VISUALISER)
									{
										visualiserMapMutex.lock();
										visualiserMap.insert(TIME_EVENT_ID_PAIR (TIME(periodicStartTime.sec() + offset.sec(), "sec"),
												eventIdVectorPeriodic.at(eventIdVectorPeriodicIt)));
										visualiserMapMutex.unlock();
									}
								}
								// get the same periodic EventID but with an updated IE value (in case it was not constant)
								eventIdVectorPeriodic = eventIdGenerator.GetEventIdForComDescr(useCaseId, readMscIt-1,
										(*eMapIt).second.first, (*eMapIt).second.second);
							}
							eventIdVectorPeriodic.clear();
						}
						startingTimeForThisComDescr = TIME(startingTimeForThisComDescr.sec() + latency.sec(), "sec");
						mut.lock();
						it = eventMap.begin();
						// Finding spare time-slot in eventMap
						while (it != eventMap.end())
						{
							it = eventMap.find(TIME(startingTimeForThisComDescr.sec() + offset.sec(), "sec"));
							if (it != eventMap.end())
								offset = TIME(offset.millisec() + 1, "millisec");
						}
						eventMap.insert(TIME_EVENT_ID_PAIR (TIME(startingTimeForThisComDescr.sec() + offset.sec(), "sec"), eventId));
						mut.unlock();
						if (VISUALISER)
						{
							visualiserMapMutex.lock();
							visualiserMap.insert(TIME_EVENT_ID_PAIR (TIME(startingTimeForThisComDescr.sec() + offset.sec(), "sec"), eventId));
							visualiserMapMutex.unlock();
						}
						LOG4CXX_TRACE (logger, "Adding EventID " << eventId
								<< " at relative time " << setprecision(20) << startingTimeForThisComDescr.sec()
								<< " to eventMap for use-case " << useCaseId << " and communication descriptor " << readMscIt);
					}
				}
			}
			eventTimerMap.erase(eMapIt);
			// Adding new starting time for the same UE

			if (ueDistDef.distribution == UNIFORM_REAL)
			{
				boost::uniform_real<> uni_dist_real (ueDistDef.uniformMin.sec(), ueDistDef.uniformMax.sec());
				boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni_real (generator, uni_dist_real);
				sTime = TIME(uni_real(), "sec");
				LOG4CXX_TRACE(logger, "Starting time distribution is UNIFORM_REAL with min = "
						<< ueDistDef.uniformMin.sec() << " and max = " << ueDistDef.uniformMax.sec()
						<< "\tValue = " << sTime.sec());
			}
			else if (ueDistDef.distribution == UNIFORM_INTEGER)
			{
				boost::uniform_int<> uni_dist_int ((int)ueDistDef.uniformMin.sec(), (int)ueDistDef.uniformMax.sec());
				boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (generator, uni_dist_int);
				sTime = TIME(uni_int(), "sec");
				LOG4CXX_TRACE(logger, "Starting time distribution is UNIFORM_INT with min = "
											<< ueDistDef.uniformMin.sec() << " and max = " << ueDistDef.uniformMax.sec()
											<< "\tValue = " << sTime.sec());
			}
			else if (ueDistDef.distribution == EXPONENTIAL)
			{
				boost::exponential_distribution<> exp_dist (ueDistDef.exponentialLambda);
				boost::variate_generator<base_generator_type&, boost::exponential_distribution<> > exponential (generator, exp_dist);
				sTime = TIME(exponential(), "sec");
			}
			else if (ueDistDef.distribution == GAUSSIAN)
			{
				boost::normal_distribution<> gau_dist (ueDistDef.gaussianMu,ueDistDef.gaussianSigma);
				boost::variate_generator<base_generator_type&, boost::normal_distribution<> > gaussian (generator, gau_dist);
				sTime = TIME(gaussian(), "sec");
			}
			else if (ueDistDef.distribution == GAMMA)
			{
				boost::gamma_distribution<> gamma_dist (ueDistDef.gammaAlpha, ueDistDef.gammaBeta);
				boost::variate_generator<base_generator_type&, boost::gamma_distribution<> > gamma (generator, gamma_dist);
				sTime = TIME(gamma(), "sec");
			}
			else if (ueDistDef.distribution == ERLANG)
			{
				boost::gamma_distribution<> erlang_dist (ueDistDef.erlangAlpha, ueDistDef.erlangBeta);
				boost::variate_generator<base_generator_type&, boost::gamma_distribution<> > erlang (generator, erlang_dist);
				sTime = TIME(erlang(), "sec");
			}
			else
			{
				LOG4CXX_ERROR(logger, "This distribution has not been implemented to calculate UE arrival times");
				pthread_exit(NULL);
			}

			clock_gettime(CLOCK_REALTIME, &ts);
			tvNsec = TIME(ts.tv_nsec, "nanosec");
			tvSec = TIME(ts.tv_sec, "sec");
			currentTime = TIME(tvSec.sec() + tvNsec.sec(), "sec");
			bool newTimeFound = false;
			while (newTimeFound == false)
			{
				if (eventTimerMap.find(TIME(currentTime.sec() + sTime.sec(), "sec")) == eventTimerMap.end())
				{
					TIME tmpTime;
					tmpTime = TIME(currentTime.sec() + sTime.sec(), "sec");
					LOG4CXX_DEBUG(logger, "Next starting time for UE " << ue
							<< " -> BS " << bs << " in " << std::setprecision(20) << tmpTime.sec() - currentTime.sec() << "s"
							<< " total time: " << currentTime.sec());
					eventTimerMap.insert(pair <TIME,BS_UE_PAIR> (tmpTime, BS_UE_PAIR (bs,ue)));
					newTimeFound = true;
				}
				else {
					TIME sTmp = TIME(1, "millisec");
					sTime = TIME(sTime.sec() + sTmp.sec(), "sec");
				}
			}
		}
	}
	LOG4CXX_ERROR (logger, "generateEventIds() thread ended");
	pthread_exit(NULL);
}
void *generateNoiseIds(void *t)
{
	HASHED_NOISE_EVENT_ID_MAP_IT hashedNoiseEventIdMapIt;
	base_generator_type generator(seed), eventIdGenerator(seed);
	boost::asio::io_service io_service;
	boost::asio::deadline_timer timer(io_service);
	TIME sTime,
	tvSec,
	tvNsec,
	currentTime;
	float tmp;
	bool timePositive = false;	/** for the while loop to check if time returned by Boost library is positive */
	EVENT_ID eId;
	timespec ts;
	for (;;)
	{
		// Ensure the next starting time calculated is positive
		while (!timePositive)
		{
			// Get next time
			if (noiseDescrStruct.distribution.distribution == UNIFORM_REAL)
			{
				boost::uniform_real<> uni_dist_real (noiseDescrStruct.distribution.latencyMinimum.sec(), noiseDescrStruct.distribution.latencyMaximum.sec());
				boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni_real (generator, uni_dist_real);
				tmp = uni_real();
			}
			else if (noiseDescrStruct.distribution.distribution == GAUSSIAN)
			{
				boost::normal_distribution<> gaussian_dist (noiseDescrStruct.distribution.gaussianMu, noiseDescrStruct.distribution.gaussianSigma);
				boost::variate_generator<base_generator_type&, boost::normal_distribution<> > gaussian(generator, gaussian_dist);
				tmp = gaussian();
			}
			else
			{
				LOG4CXX_ERROR(logger, "Noise distribution " << noiseDescrStruct.distribution.distribution << " has not been implemented");
				break;
			}

			if (tmp >= 0)
				timePositive = true;
		}
		timePositive = false; // reset this boolean
		sTime = TIME(tmp, "sec");
		LOG4CXX_DEBUG(logger, "Calculated distribution time for NoiseID = " << sTime.sec());
		//Get Noise EventID
		boost::uniform_int<> uni_dist_int (0, hashedNoiseEventIdMap.size() - 1);
		boost::variate_generator<base_generator_type&, boost::uniform_int<> > uni_int (eventIdGenerator, uni_dist_int);
		hashedNoiseEventIdMapIt = hashedNoiseEventIdMap.find(uni_int());
		clock_gettime(CLOCK_REALTIME, &ts);
		tvNsec = TIME(ts.tv_nsec, "nanosec");
		tvSec = TIME(ts.tv_sec, "sec");
		currentTime = TIME(tvSec.sec() + tvNsec.sec(), "sec");
		TIME offset = TIME(1, "nanosec");
		mut.lock();
		//Adding NoiseID to shared eventMap - make sure to find unique time
		while (eventMap.find(TIME(currentTime.sec() + sTime.sec(), "sec")) != eventMap.end())
			sTime = TIME(sTime.sec() + offset.sec(),"sec");
		eventMap.insert(TIME_EVENT_ID_PAIR (TIME(currentTime.sec() + sTime.sec(), "sec"), (*hashedNoiseEventIdMapIt).second));
		mut.unlock();
		if (VISUALISER)
		{
			visualiserMapMutex.lock();
			visualiserMap.insert(TIME_EVENT_ID_PAIR (TIME(currentTime.sec() + sTime.sec(), "sec"), (*hashedNoiseEventIdMapIt).second));
			visualiserMapMutex.unlock();
		}
		LOG4CXX_TRACE(logger, "Uncorrelated noise EventID added to eventMap at time " << std::setprecision(20) << currentTime.sec() + sTime.sec() << "s");
		LOG4CXX_DEBUG(logger, "Waiting " << sTime.sec() << " seconds before generating next uncorrelated noise EventID");
		timer.expires_from_now(boost::posix_time::microseconds(sTime.microsec()));
		timer.wait();
	}
	LOG4CXX_ERROR (logger, "generateNoiseIds() thread ended");
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
	TIME printingRateTime;
	unsigned int countEventIds = 0, countEventIdsTotal = 0;
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
	printingRateTime = TIME(s+1.0, "sec");
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
				file << std::setprecision(PRECISION) << (double)(currentTime.sec() - emulationStartTime.sec()) << "\t" << payload << endl;
			if (VISUALISER == true)
				LOG4CXX_TRACE(logger, "Sending EventID " << payload << " to OpenMSC visualiser");
			LOG4CXX_TRACE(logger, "Sending EventID " << payload << " / EventID(s) in map: " << eventMap.size()-1);
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
			// count Numbers of sent EventIDs
			if (printingRateTime.sec() < currentTime.sec())
			{
				printingRateTime = TIME(currentTime.sec() + 1.0, "sec");
				countEventIdsTotal += countEventIds;
				if (PRINT_EVENT_ID_RATE)
					LOG4CXX_INFO(logger, "EventID Rate: " << countEventIds
							<< "/s\tTotal = " << countEventIdsTotal
							<< "\tAverage Rate: " << floor(countEventIdsTotal / (currentTime.sec() - emulationStartTime.sec())) << "IDs/s");
				countEventIds = 0;
			}
			else
				countEventIds++;
			// Check if stream has already reached it requested size
			if (AUTOMATICALLY_STOP_SENDING && stopRate < countEventIdsTotal)
			{
				if (streamToFileFlag)
					file.close();

				LOG4CXX_INFO (logger, stopRate << " EventIDs have been sent. OpenMSC will be terminated");
				LOG4CXX_INFO (logger, "Average EventID rate: " << countEventIdsTotal / (currentTime.sec() - emulationStartTime.sec()) << "IDs/s");
				exit(0);
			}
		}
	}
	if (streamToFileFlag)
		file.close();

	LOG4CXX_ERROR (logger, "sendEventIds() thread ended");
	pthread_exit(NULL);
}

/**
 * Visualiser
 *
 * This function visualises the stream
 *
 * @param pointer to Thread Identifier
 * @return void
 */
void *visualiser(void *t)
{
	boost::asio::io_service io_service;
	boost::asio::deadline_timer timer(io_service);
	Visualiser visualiser;
	visualiser.Initialise(logger, visualiserWindowSize);
	timespec ts;
	for (;;)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		TIME tvNsec(ts.tv_nsec, "nanosec");
		TIME tvSec(ts.tv_sec, "sec");
		double s = tvSec.sec() + tvNsec.sec();
		TIME currentTime(s, "sec");
		visualiserMapMutex.lock();
		// Clean Visualiser map with IDs older than visualiser window size
		visualiser.UpdateEventIdMap(&visualiserMap, currentTime);
		visualiserMapMutex.unlock();
		visualiser.UpdatePlot(currentTime);
		timer.expires_from_now(boost::posix_time::millisec (visualiserUpdateInterval));
		timer.wait();
	}

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
		if (!(openmscConfig.lookupValue("numOfUesPerBs", *numOfUesPerBs_)
				&& openmscConfig.lookupValue("numOfBss", *numOfBss_)
				&& openmscConfig.lookupValue("ueActivity-Dist", dist)))
		{
			LOG4CXX_ERROR (logger, "Parameters numOfUesPerBs, numOfBss and/or ueActivity-Dist could not be read");
			return false;
		}

		if (strcmp(dist,"constant") == 0)
		{
			float t;
			ueDistDef.distribution = CONSTANT;
			openmscConfig.lookupValue("ueActivity-Dist-Value", t);
			ueDistDef.constantLatency = TIME(t, "ms");
		}
		else if (strcmp(dist,"exponential") == 0)
		{
			ueDistDef.distribution = EXPONENTIAL;
			openmscConfig.lookupValue("ueActivity-Dist-Lambda", ueDistDef.exponentialLambda);
		}
		else if (strcmp(dist,"uniform_real") == 0)
		{
			float min, max;
			ueDistDef.distribution = UNIFORM_REAL;
			openmscConfig.lookupValue("ueActivity-Dist-Min", min);
			ueDistDef.uniformMin = TIME(min,"sec");
			openmscConfig.lookupValue("ueActivity-Dist-Max", max);
			ueDistDef.uniformMax = TIME(max,"sec");
			LOG4CXX_DEBUG(logger, "Distribution: uniform_real\tMin = " << ueDistDef.uniformMin.sec() << "\tMax = " << ueDistDef.uniformMax.sec());
		}
		else if (strcmp(dist,"uniform_int") == 0)
		{
			int min, max;
			ueDistDef.distribution = UNIFORM_INTEGER;
			openmscConfig.lookupValue("ueActivity-Dist-Min", min);
			ueDistDef.uniformMin = TIME(min,"sec");
			openmscConfig.lookupValue("ueActivity-Dist-Max", max);
			ueDistDef.uniformMax = TIME(max,"sec");
			LOG4CXX_DEBUG(logger, "Distribution: uniform_int\tMin = " << ueDistDef.uniformMin.sec() << "\tMax = " << ueDistDef.uniformMax.sec());
		}
		else if (strcmp(dist,"gamma") == 0)
		{
			ueDistDef.distribution = GAMMA;
			openmscConfig.lookupValue("ueActivity-Dist-Alpha", ueDistDef.gammaAlpha);
			openmscConfig.lookupValue("ueActivity-Dist-Beta", ueDistDef.gammaBeta);
		}
		else if (strcmp(dist,"erlang") == 0)
		{
			ueDistDef.distribution = ERLANG;
			openmscConfig.lookupValue("ueActivity-Dist-Alpha", ueDistDef.erlangAlpha);
			openmscConfig.lookupValue("ueActivity-Dist-Beta", ueDistDef.erlangBeta);
		}
		else if (strcmp(dist,"gaussian") == 0)
		{
			ueDistDef.distribution = GAUSSIAN;
			openmscConfig.lookupValue("ueActivity-Dist-Mu", ueDistDef.gaussianMu);
			openmscConfig.lookupValue("ueActivity-Dist-Sigma", ueDistDef.gaussianSigma);
		}
		else
		{
			LOG4CXX_ERROR(logger,"ueActivity-Dist comprises unknown value!");
			return false;
		}
		//Checking seed value
		if(!openmscConfig.lookupValue("seed", seed))
		{
			LOG4CXX_ERROR(logger,"Seed value not given in openmsc.cfg");
			return false;
		}
		// Read information elements
		try
		{
			const Setting &informationElements = root["openmscConfig"]["informationElements"];
			int count = informationElements.getLength();
			LOG4CXX_DEBUG(logger, count << " information element(s) found in openmsc.cfg");
			for(int i = 0; i < count; ++i)
			{
				const Setting &ie = informationElements[i];
				string ieName, ieDist;
				if(!(ie.lookupValue("ieName", ieName) && ie.lookupValue("ieDist", ieDist)))
				{
					LOG4CXX_ERROR (logger, "Could not read ieName (" << ieName << ") and/or ieDist (" << ieDist << ") in openmsc.cfg");
					return false;
				}
				INFORMATION_ELEMENT_DESCRIPTION_STRUCT ieDescrStruct;
				if (ieDist.find("gaussian") != string::npos)
				{
					string ieDistMu, ieDistSigma;
					ieDescrStruct.ieValueDistDef.distribution = GAUSSIAN;
					if (!(ie.lookupValue("ieDistMu", ieDistMu) && ie.lookupValue("ieDistSigma", ieDistSigma)))
					{
						LOG4CXX_ERROR(logger, "ieDistMu and/or ieDistSigma could not be read from openmsc.cfg");
						return false;
					}
					ieDescrStruct.ieValueDistDef.gaussianMu = atof(ieDistMu.c_str());
					ieDescrStruct.ieValueDistDef.gaussianSigma = atof(ieDistSigma.c_str());
				}
				else if (ieDist.find("constant") != string::npos)
				{
					string ieDistValue;
					ieDescrStruct.ieValueDistDef.distribution = CONSTANT;
					if (!(ie.lookupValue("ieDistValue", ieDistValue)))
					{
						LOG4CXX_ERROR(logger, "ieDistValue could not be read from openmsc.cfg");
						return false;
					}
					ieDescrStruct.ieValueDistDef.constantLatency = TIME(atof(ieDistValue.c_str()), "millisec");
				}
				else
				{
					LOG4CXX_ERROR (logger, "Unknown 'ieDist' distribution value specified in openmsc.cfg");
						return false;
				}
				readMsc.AddInformationElementDescription(INFORMATION_ELEMENT_DESCRIPTION_PAIR (ieName, ieDescrStruct));
			}
		}
		catch(const SettingNotFoundException &nfex)
		{
			LOG4CXX_ERROR(logger, "Reading informationElements definitions from openmsc.cfg failed");
			return false;
		}
		// Read noise config (if it exists)
		try
		{
			const Setting &noiseUncorrelated = root["openmscConfig"]["noise"]["uncorrelated"];
			int count = noiseUncorrelated.getLength();

			LOG4CXX_DEBUG(logger, "Uncorrelated noise definition found");

			for(int i = 0; i < count; ++i)
			{
				const Setting &noise = noiseUncorrelated[i];
				string dist;
				EVENT_ID eventIdRangeMin, eventIdRangeMax;
				const char * distMin, * distMax;

				if(!(noise.lookupValue("distOccurrence", dist)
						&& noise.lookupValue("eventIdRangeMin", eventIdRangeMin)
						&& noise.lookupValue("eventIdRangeMax", eventIdRangeMax)))
				{
					LOG4CXX_ERROR (logger, "Could not read distOccurrence, eventIdRangeMin and/or eventIdRangeMax");
					return false;
				}

				noiseDescrStruct.eventIdRangeMin = eventIdRangeMin;
				noiseDescrStruct.eventIdRangeMax = eventIdRangeMax;
				int range = atoi(noiseDescrStruct.eventIdRangeMax.c_str()) - atoi(noiseDescrStruct.eventIdRangeMin.c_str());
				// Check if noise EventIDs are largern than int type

				if (atoll(noiseDescrStruct.eventIdRangeMax.c_str()) > MAX_INT)
				{
					LOG4CXX_ERROR(logger, "Noise EventID range is larger than " << MAX_INT
							<< " and cannot be handled by OpenMSC at the moment.");
					return false;
				}
				for (int i = 0; i < range; i++)
				{
					ostringstream convert;
					convert << atoi(noiseDescrStruct.eventIdRangeMin.c_str()) + i;
					hashedNoiseEventIdMap.insert(pair<int, EVENT_ID> (i,convert.str()));
				}
				// Generating the time for the next random noise EventID
				if (dist.find("uniform_real") != string::npos)
				{
					if (noise.lookupValue("distOccurrenceMin", distMin) && noise.lookupValue("distOccurrenceMax", distMax))
					{
						noiseDescrStruct.distribution.distribution = UNIFORM_REAL;
						noiseDescrStruct.distribution.latencyMinimum = TIME(atof(distMin), "sec");
						noiseDescrStruct.distribution.latencyMaximum = TIME(atof(distMax), "sec");
					}
					else
					{
						LOG4CXX_ERROR (logger, "Cannot read distOccurrenceMin (" << distMin << ") and/or distOccurrenceMax (" << distMax << ") parameter for UNIFORM_REAL distribution in openmsc.cfg");
						return false;
					}
				}
				else if (dist.find("gaussian") != string::npos)
				{
					string mu, sigma;
					if (noise.lookupValue("distOccurrenceMu", mu) && noise.lookupValue("distOccurrenceSigma", sigma))
					{
						noiseDescrStruct.distribution.distribution = GAUSSIAN;
						noiseDescrStruct.distribution.gaussianMu = atof(mu.c_str());
						noiseDescrStruct.distribution.gaussianSigma = atof(sigma.c_str());
					}
					else
					{
						LOG4CXX_ERROR (logger, "Cannot read mu ("
								<< mu << ") and/or sigma ("
								<< sigma << ") parameter for GAUSSIAN distribution in openmsc.cfg");
						return false;
					}
				}
				else {
					LOG4CXX_ERROR (logger, "Starting time distribution " << dist << " has not been implemented for uncorrelated noise EventIDs");
					return false;
				}
			}
		}
		catch(const SettingNotFoundException &nfex)
		{
			LOG4CXX_INFO (logger, "Noise declaration in openmsc.cfg either not given or could not be read");
		}
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
	pthread_t threads[4];
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
		{ 0, 'r', 0, 0, "Print 'EventIDs per second' rate and total # of EventIDs to stdout using INFO logging level"},
		{ "TCP", 't', 0, 0, "Using IPv4 over TCP to communicate with destination module"},
		{ "UDP", 'u', 0, 0, "Using IPv4 over UDP to communicate with destination module"},
		{ "port", 'p', "<PORT>", 0, "Port number of the receiving module"},
		{ "ip", 'i', "<IPv4 Address>", 0, "IP address of the receiving module"},
		{ 0, 'f', 0, 0, "Write EventIDs to file 'eventStream.tsv'"},
		{ "visualiser", 'v', "<NUMBER>", 0, "Enable real-time visualiser with a window size in seconds"},
		{ "vInt", 'w', "<NUMBER>", 0, "Set update interval to customised value"},
		{ "debug", 'd', "<LEVEL>", 0, "Debug level (ERROR|INFO|DEBUG|TRACE)" },
		{ 0, 's', "<NUMBER>", 0, "Stop OpenMSC after it sent <NUMBER> EventIDs"},
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

	dictionary.Init();
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

	LOG4CXX_INFO(logger, "Creating generateEventIds thread");
	rc = pthread_create(&threads[0], NULL, generateEventIds, (void *)i );

	if (rc){
		LOG4CXX_ERROR(logger,"Unable to create generateEventIds thread, " << rc);
		exit(-1);
	}

	LOG4CXX_INFO(logger, "Creating sendStream thread");
	rc = pthread_create(&threads[1], NULL, sendStream, (void *)i );

	if (rc){
		LOG4CXX_ERROR(logger,"Unable to create sendStream thread, " << rc);
		exit(-1);
	}

	LOG4CXX_INFO(logger, "Creating generateNoiseIds thread");
	rc = pthread_create(&threads[1], NULL, generateNoiseIds, (void *)i );

	if (rc){
		LOG4CXX_ERROR(logger,"Unable to create generateNoiseIds thread, " << rc);
		exit(-1);
	}

	if (VISUALISER)
	{
		LOG4CXX_INFO(logger, "Creating visualiser thread");
		rc = pthread_create(&threads[1], NULL, visualiser, (void *)i );

		if (rc)
		{
			LOG4CXX_ERROR(logger,"Unable to create visualiser thread, " << rc);
			exit(-1);
		}
	}
	pthread_exit(NULL);

	return(EXIT_SUCCESS);
}
