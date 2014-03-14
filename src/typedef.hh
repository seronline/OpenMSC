/**
 * @file typedef.hh
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

#include <map>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <boost/asio/time_traits.hpp>
#include <boost/random/linear_congruential.hpp>
#include "enum.hh"
#include "time.hh"

using namespace std;

/**
 * \typedef IP_ADDRESS
 * IP address of the EventID receiver (OpenMSC receiver)
 */
typedef string IP_ADDRESS;
/**
 * \typedef PORT
 * Port number of the EventID receiver (OpenMSC receiver)
 */
typedef string PORT;
/**
 * \typedef DEBUG_LEVEL
 * Integer representation of the debug level
 */
typedef int DEBUG_LEVEL;
/**
 * \typedef NETWORK_ELEMENT
 * \brief string representation of a network element
 */
typedef string NETWORK_ELEMENT;

/**
 * \typedef MESSAGE_TYPE
 * \brief string representation of a protocol type
 */
typedef string PROTOCOL_TYPE;

/**
 * \typedef PRIMITIVE_NAME
 * \brief string representation of a primitive's name
 */
typedef string PRIMITIVE_NAME;

/**
 * \typedef IDENTIFIER
 * \brief Integer representation of a network element, message type or value
 */
typedef int IDENTIFIER;

/**
 * \typedef EVENT_ID
 * \brief Numeric identifier for a particular EventID
 */
typedef string EVENT_ID;
/**
 * \typedef DICTIONARY_FILE_NAME
 * Filename for an OpenMSC dictionary
 */
typedef const char* DICTIONARY_FILE_NAME;
/**
 * \typedef EVENT_ID_VECTOR
 * A vector of EventIDs
 */
typedef vector <EVENT_ID> EVENT_ID_VECTOR;
/**
 * \typedef INFORMATION_ELEMENT
 * \brief String representation of an information element
 */
typedef string INFORMATION_ELEMENT;
/**
 * \typedef BS_ID
 * \brief Base-Station Identifier
 */
typedef int BS_ID;

/**
 * \typedef UE_ID
 * \brief User Equipment Identifier
 */
typedef int UE_ID;

/**
 * \typedef NETWORK_ELEMENTS_COUNTER
 *  Counter for total number of network elements defined in openmsc.msc
 */
typedef unsigned int NETWORK_ELEMENTS_COUNTER;

/**
 * \typedef PROTOCOL_TYPES_COUNTER
 * Counter for total number of protocol types defined in openmsc.msc
 */
typedef unsigned int PROTOCOL_TYPES_COUNTER;

/**
 * \typedef PRIMITIVE_NAMES_COUNTER
 * Counter for total number of protocol types defined in openmsc.msc
 */
typedef unsigned int PRIMITIVE_NAMES_COUNTER;
/**
 * \typedef INFORMATION_ELEMENTS_COUNTER
 * Counter for total number of information elements defined in openmsc.msc
 */
typedef unsigned int INFORMATION_ELEMENTS_COUNTER;
/**
 * \typedef USE_CASE
 * \brief Unique integer representation of the use-case (Success || Failure)
 */
typedef unsigned int USE_CASE_ID;

/**
 * \typedef PROBABILITY
 * \brief Float number of probability
 */
typedef float PROBABILITY;

/**
 * \typedef TIME
 * \brief Numeric (float) representation of a time
 */
//typedef float TIME;

/**
 * \typedef LATENCY
 * The latency of a communication between two network elements
 */
typedef float LATENCY;
/**
 * \typedef LATENCY_AVERAGE
 * Floating point numeric representation of the average communication latency between two network elements
 */
typedef float LATENCY_AVERAGE;

/**
 * \typedef LATENCY_MINIMUM
 * Floating point numeric representation of the minimum communication latency between two network elements
 */
typedef float LATENCY_MINIMUM;

/**
 * \typedef LATENCY_MAXIMUM
 * Floating point numeric representation of the maximum communication latency between two network elements
 */
typedef float LATENCY_MAXIMUM;

/**
 * \typedef INFORMATION_ELEMENT_VECTOR
 * \brief vector of strings for holding the values from the MSC file
 */
typedef vector <INFORMATION_ELEMENT> INFORMATION_ELEMENT_VECTOR;

/**
 * \typedef DISTRIBUTION
 * \brief integer representation of a distribution using the latencyDistributionEnum enumeration declaration in enum.hh
 */
typedef int DISTRIBUTION;
/**
 * \typedef LATENCY_DESCRIPTION_STRUCT
 * \brief Struct describing the communication latency between two network elements.
 *
 * Note, all fields are optional and depend on what distribution was chosen. The names for the type definitions were chosen from the boost definitions for distributions: http://www.boost.org/doc/libs/1_54_0/libs/math/doc/html/dist.html
 */
typedef struct distributionDefinition {
	DISTRIBUTION distribution;
	TIME latencyAverage;					/** Average latency of the communication between source and destination */
	TIME latencyMinimum;
	TIME latencyMaximum;
	TIME uniformMin;						/** Start time for the uniform distribution */
	TIME uniformMax;						/** Stop time for the uniform distribution */
	TIME constantLatency;					/** Fixed time for latency of a communication descriptor */
	float gammaAlpha;						/** scale gamma parameter in seconds */
	float gammaBeta;						/** shape gamma parameter (probability) */
	int erlangAlpha;						/** scale erlang parameter in seconds */
	int erlangBeta;							/** shape erlang parameter (probability) */
	float exponentialLambda;				/** lambda parameter for exponential distribution */
	float gaussianMu;						/** mean parameter [s] for gaussian distribution */
	float gaussianSigma;					/** standard deviation [s] for gaussian distribution */
} DISTRIBUTION_DEFINITION_STRUCT;

/**
 * \typedef COMMUNICATION_DESCRIPTION_STRUCT
 * \brief struct to hold data from a single unidirectional or broadcast communication between two or n network elements
 */
typedef struct communicationDescription {
	NETWORK_ELEMENT source,							/** Unique name of the source network element */
					destination;					/** Unique name of the destination network element */
	PROTOCOL_TYPE protocolType;						/** Type of network protocol used */
	PRIMITIVE_NAME primitiveName;					/** Message type of the communication */
	INFORMATION_ELEMENT_VECTOR informationElements;	/** Vector of information elements */
	DISTRIBUTION_DEFINITION_STRUCT latencyDescription;	/** Struct holding information about the communication latency between source and destination */
} COMMUNICATION_DESCRIPTION_STRUCT;

/**
 * \typedef NOISE_DESCRIPTION_STRUCT
 * \brief struct to hold noise definition from openmsc.cfg 'noise = {uncorrelated()}'
 */
typedef struct noiseDescription {
	EVENT_ID	eventIdRangeMin,
				eventIdRangeMax;
	DISTRIBUTION_DEFINITION_STRUCT distribution;
} NOISE_DESCRIPTION_STRUCT;
/**
 * \typedef MSC_LINE_VECTOR
 * \brief string vector for a line from the MSC file
 */
typedef vector <string> MSC_LINE_VECTOR;

/**
 * \typedef COMMUNICATION_DESCRIPTION_VECTOR
 * \brief vector of COMMUNICATION_DESCRIPTION_STRUCTs
 */
typedef vector <COMMUNICATION_DESCRIPTION_STRUCT> COMMUNICATION_DESCRIPTION_VECTOR;

/**
 * \typedef BS_UE_PAIR
 * \brief std::pair with the numeric representations of the base-station identifier and the user equipment identifier
 */
typedef pair <BS_ID,UE_ID> BS_UE_PAIR;

/**
 * \typedef TIME_EVENT_ID_PAIR
 * \brief TIME <> EVENT_ID pair
 */
typedef pair <TIME,EVENT_ID> TIME_EVENT_ID_PAIR;

/**
 * \typedef EVENT_MAP
 * \brief TIME <> EVENT_ID map
 */
typedef map <TIME,EVENT_ID> EVENT_MAP;

/**
 * \typedef EVENT_TIMER_MAP
 * \brief TIME <> BS_UE_PAIR map
 */
typedef map <TIME,BS_UE_PAIR> EVENT_TIMER_MAP;
/**
 * \typedef HASHED_NOISE_EVENT_ID_MAP
 * \brief Hashed integer number for string Noise EventID representation
 */
typedef map <int, EVENT_ID> HASHED_NOISE_EVENT_ID_MAP;
/**
 * \typedef HASHED_NOISE_EVENT_ID_MAP_IT
 * \brief Iterator for HASHED_NOISE_EVENT_ID_MAP
 */
typedef map <int, EVENT_ID>::iterator HASHED_NOISE_EVENT_ID_MAP_IT;
/**
 * \typedef EVENT_MAP_IT
 * \brief Iterator for TIME <> EVENT_ID map
 */
typedef map <TIME,EVENT_ID>::iterator EVENT_MAP_IT;

/**
 * \typedef EVENT_TIMER_MAP_IT
 * \brief iterator for TIME <> BS_UE_PAIR map
 */
typedef map <TIME,BS_UE_PAIR>::iterator EVENT_TIMER_MAP_IT;

/**
 * \typedef NETWORK_ELEMENTS_MAP
 * std::map of network elements as keys and their corresponding unique identifiers
 */
typedef map <NETWORK_ELEMENT, IDENTIFIER> NETWORK_ELEMENTS_MAP;

/**
 * \typedef NETWORK_ELEMENTS_MAP_IT
 * \brief std::map iterator for NETWORK_ELEMENTS_MAP
 */
typedef map <NETWORK_ELEMENT, IDENTIFIER>::iterator NETWORK_ELEMENTS_MAP_IT;

/**
 * \typedef PRIMITIVE_NAMES_MAP
 * std::map of primitive names as keys and their corresponding unique identifiers
 */
typedef map <PRIMITIVE_NAME, IDENTIFIER> PRIMITIVE_NAMES_MAP;

/**
 * \typedef PRIMITIVE_NAMES_MAP_IT
 * \brief std::map iterator for PRIMITIVE_NAMES_MAP
 */
typedef map <PRIMITIVE_NAME, IDENTIFIER>::iterator PRIMITIVE_NAMES_MAP_IT;

/**
 * \typedef PROTOCOL_TYPES_MAP
 * std::map of protocol types as keys and their corresponding unique identifiers
 */
typedef map <PROTOCOL_TYPE, IDENTIFIER> PROTOCOL_TYPES_MAP;

/**
 * \typedef PROTOCOL_TYPES_MAP_IT
 * \brief std::map iterator for PROTOCOL_TYPES_MAP
 */
typedef map <PROTOCOL_TYPE, IDENTIFIER>::iterator PROTOCOL_TYPES_MAP_IT;
/**
 * \typedef INFORMATION_ELEMENTS_MAP
 * std::map of information elements as keys and their corresponding unique identifiers
 */
typedef map <INFORMATION_ELEMENT, IDENTIFIER> INFORMATION_ELEMENTS_MAP;

/**
 * \typedef INFORMATION_ELEMENT_MAP_IT
 * \brief std::map iterator for INFORMATION_ELEMENTS_MAP
 */
typedef map <INFORMATION_ELEMENT, IDENTIFIER>::iterator INFORMATION_ELEMENTS_MAP_IT;

/**
 * \typedef USE_CASE_DESCRIPTION_MAP
 * \brief std::map to hold vector of communications between NEs for a particular use-case
 */
typedef map <USE_CASE_ID, COMMUNICATION_DESCRIPTION_VECTOR> USE_CASE_DESCRIPTION_MAP;

/**
 * \typedef USE_CASE_DESCRIPTION_MAP_IT
 * \brief std::map iterator for USE_CASE_DESCRIPTION_MAP
 */
typedef map <USE_CASE_ID, COMMUNICATION_DESCRIPTION_VECTOR>::iterator USE_CASE_DESCRIPTION_MAP_IT;

/**
 * \typedef USE_CASE_PROBABILITY_MAP
 * \brief std::map to hold occurrence probability for each use-case
 *
 * The probability is read from the openmsc.msc file
 */
typedef map <USE_CASE_ID, PROBABILITY> USE_CASE_PROBABILITY_MAP;
/**
 * \typedef USE_CASE_PROBABILITY_MAP_IT
 * \brief std::map iterator for USE_CASE_PROBABILITY_MAP
 *
 * The probability is read from the openmsc.msc file
 */
typedef map <USE_CASE_ID, PROBABILITY>::iterator USE_CASE_PROBABILITY_MAP_IT;
/**
 * \typedef Time
 * \brief boost::posix_time definition
 */
typedef boost::posix_time::ptime Time;

/**
 * \typedef base_generator_type
 * \brief Required for seed calculation
 */
typedef boost::minstd_rand base_generator_type;

/**
 * \struct time_t_traits
 * \brief Helper struct for boost::time
 */
typedef boost::asio::time_traits<boost::posix_time::ptime> time_traits_t;

#define PRECISION 6
