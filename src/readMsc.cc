/**
 * @file msc.cc
 * @author Sebastian Robitzsch <srobitzsch@gmail.com>
 * @version 0.1
 *
 * @section LICENSE
 *
 * OpenMSC - MSCgen-Based Control-Plane Traffic Emulator
 *
 * Copyright (C) 2014 Sebastian Robitzsch
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
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <iomanip>
#include "readMsc.hh"

using namespace std;

int ReadMsc::ReadMscConfigFile ()
{
	ifstream inputFile;	/** char file input stream */
	string line;		/** string to hold a line from the MSC file */
	USE_CASE_ID useCaseId = 0; /** Integer use-case identifier initialised with 0*/

	inputFile.open ("openmsc.msc");

	if (!inputFile.is_open())
	{
		printf("openmsc.msc could not be opened\n");

		return(EXIT_FAILURE);
	}

	while (getline (inputFile,line))
	{
		MSC_LINE_VECTOR lineVector1,lineVector2;
		boost::algorithm::split_regex(lineVector1, line, boost::regex ("---"));

		if (lineVector1.size() > 1)
		{
			MSC_LINE_VECTOR caseConfig;
			cout << lineVector1.size() << line << endl;
			boost::split (caseConfig, line, boost::is_any_of("\""));

			// checking for use-case
			for (unsigned i = 0; i < caseConfig.size(); i++)
			{
				if (caseConfig.at(i) == "Success")
				{
					cout << "Success use-case found\n";
					useCaseId++;
				}
				else if (caseConfig.at(i) == "Failure")
				{
					cout << "New failure use-case found\n";
					useCaseId++;
				}
			}

			// TODO reading probabilities for use-cases
		}

		// Get message types and latencies for communications
		boost::algorithm::split_regex(lineVector1, line, boost::regex ("=>"));
		boost::algorithm::split_regex(lineVector2, line, boost::regex ("<="));

		MSC_LINE_VECTOR lineTmp;

		if (lineVector1.size() > 1)
			lineTmp = lineVector1;
		else if (lineVector2.size() > 1)
			lineTmp = lineVector2;
		else
			lineTmp.clear();

		if (lineTmp.size() > 0)
		{
			NETWORK_ELEMENT source, destination;
			MSC_LINE_VECTOR mscLine;
			PRIMITIVE_NAME primitiveName;
			PROTOCOL_TYPE protocolType;
			INFORMATION_ELEMENT_VECTOR informationElements;
			LATENCY_DESCRIPTION_STRUCT latencyDescription;

			if (!ExtractDataFromLine(lineTmp, &source, &destination, &protocolType, &primitiveName, &informationElements, &latencyDescription))
				return(EXIT_FAILURE);

			AddCommunicationDescription(useCaseId, source, destination, protocolType, primitiveName, informationElements, latencyDescription);
		}
	}

	inputFile.close();

	return(EXIT_SUCCESS);
}

int ReadMsc::GetMscLength(USE_CASE_ID useCaseId)
{
	USE_CASE_DESCRIPTION_MAP_IT it;
	it = useCaseDescrMap.find(useCaseId);

	if (it == useCaseDescrMap.end())
		return 0;

	return (*it).second.size();
}

int ReadMsc::GetNumOfUseCases()
{
	return useCaseDescrMap.size();
}

bool ReadMsc::AddCommunicationDescription (USE_CASE_ID uc,
		NETWORK_ELEMENT src,
		NETWORK_ELEMENT dst,
		PROTOCOL_TYPE protType,
		PRIMITIVE_NAME primName,
		INFORMATION_ELEMENT_VECTOR infElements,
		LATENCY_DESCRIPTION_STRUCT latencyDescription)
{
	USE_CASE_DESCRIPTION_MAP_IT useCaseDescrMapIt;
	COMMUNICATION_DESCRIPTION_STRUCT commDescrStruct;
	commDescrStruct.source = src;
	commDescrStruct.destination = dst;
	commDescrStruct.protocolType = protType;
	commDescrStruct.primitiveName = primName;
	commDescrStruct.informationElements = infElements;

	CheckNetworkElementIdentifier(src);
	CheckNetworkElementIdentifier(dst);
	CheckProtocolTypeIdentifier(protType);
	CheckPrimitiveNameIdentifier(primName);

	for (unsigned int i = 0; i < commDescrStruct.informationElements.size(); i++)
		CheckInformationElementIdentifier(commDescrStruct.informationElements.at(i));

	// New use-case
	if (useCaseDescrMap.find(uc) == useCaseDescrMap.end())
	{
		COMMUNICATION_DESCRIPTION_VECTOR comDescrV;
		comDescrV.insert(comDescrV.begin(),commDescrStruct);
		useCaseDescrMap.insert(pair <USE_CASE_ID, COMMUNICATION_DESCRIPTION_VECTOR> (uc, comDescrV));
		useCaseDescrMapIt = useCaseDescrMap.find(uc);
		cout << "New use-case map key " << uc << " created:";
	}
	// Add to existing use-case
	else
	{
		useCaseDescrMapIt = useCaseDescrMap.find(uc);
		// Add new communication description at the end of the vector
		(*useCaseDescrMapIt).second.insert((*useCaseDescrMapIt).second.end(),commDescrStruct);
		cout << "Communication description added to existing use-case with ID " << uc;
		//cout << "Use-case map entry " << (*useCaseDescrMapIt).first
		//		<< " extended with new value field " << (*useCaseDescrMapIt).second.size() << endl;
	}

	cout << ":\n\tSource:         " << (*useCaseDescrMapIt).second.at((*useCaseDescrMapIt).second.size() - 1).source
			<< "\n\tDestination:    " << (*useCaseDescrMapIt).second.at((*useCaseDescrMapIt).second.size() - 1).destination
			<< "\n\tProtocol Type:  " << (*useCaseDescrMapIt).second.at((*useCaseDescrMapIt).second.size() - 1).protocolType
			<< "\n\tPrimitive Name: " << (*useCaseDescrMapIt).second.at((*useCaseDescrMapIt).second.size() - 1).primitiveName
			<< "\n\tNumber of IEs:  " << (*useCaseDescrMapIt).second.at((*useCaseDescrMapIt).second.size() - 1).informationElements.size()
			<< endl;
	return true;
}

bool ReadMsc::ExtractDataFromLine(MSC_LINE_VECTOR line,
		NETWORK_ELEMENT *src_,
		NETWORK_ELEMENT *dst_,
		PROTOCOL_TYPE *protocolType_,
		PRIMITIVE_NAME *primitiveName_,
		INFORMATION_ELEMENT_VECTOR *informationElements_,
		LATENCY_DESCRIPTION_STRUCT *latencyDescription_)
{
	MSC_LINE_VECTOR lineTmp, lineTmp2, lineTmp3, lineTmp4, lineTmp5, lineTmp6;

	// Network elements
	boost::split (lineTmp, line.at(0), boost::is_any_of(" "));

	if (lineTmp.size() > 0)
	{
		MSC_LINE_VECTOR lineTmp2;
		// Checking if first network element was written with a trailing tab
		boost::split (lineTmp2, lineTmp.at(0), boost::is_any_of("\t"));

		if (lineTmp2.size() > 0)
			*src_ = lineTmp2.at(1);
		else
			*src_ = lineTmp.at(0);
	}
	else
		return false;

	lineTmp.clear();
	boost::split (lineTmp, line.at(1), boost::is_any_of(" "));

	if (lineTmp.size() > 0)
		*dst_ = lineTmp.at(1);
	else
		return false;

	// Protocol type && primitive name
	lineTmp.clear();
	boost::split(lineTmp, line.at(1), boost::is_any_of("\""));

	if (lineTmp.size() > 0)
	{
		// Protocol type
		boost::split(lineTmp2, lineTmp.at(1), boost::is_any_of("-"));
		*protocolType_ = lineTmp2.at(0);
		// Primitive name
		boost::split(lineTmp3, lineTmp2.at(1), boost::is_any_of("("));

		if (lineTmp3.size() > 1)
			*primitiveName_ = lineTmp3.at(0);
		else
		{
			cout << "ERROR: No information elements given in openmsc.msc file: " << lineTmp.at(1) << endl;
			return false;
		}
		// Information elements
		lineTmp3.clear();
		boost::split(lineTmp3, lineTmp2.at(1), boost::is_any_of("("));

		boost::split(lineTmp4, lineTmp3.at(1), boost::is_any_of(")"));
		boost::split(lineTmp5, lineTmp4.at(0), boost::is_any_of(","));

		for (unsigned int i = 0; i < lineTmp5.size(); i++)
			(*informationElements_).insert((*informationElements_).end(), lineTmp5.at(i));

		lineTmp2.clear();
		lineTmp3.clear();
		lineTmp4.clear();
		lineTmp5.clear();
	}
	else
		return false;

	// Latency description
	boost::split (lineTmp2, line.at(1), boost::is_any_of("#"));

	if (lineTmp2.size() > 1)
		boost::split (lineTmp3, lineTmp2.at(1), boost::is_any_of(" "));
	else
	{
		cout << "ERROR: no data given: " << line.at(1) << endl;

		return false;
	}

	for (unsigned int i = 0; i < lineTmp3.size(); i++)
	{
		if (lineTmp3.at(i) == "latencyDist")
		{
			boost::algorithm::split_regex(lineTmp4, lineTmp2.at(1), boost::regex ("latencyDist"));
			boost::split(lineTmp5, lineTmp4.at(1), boost::is_any_of("{"));
			lineTmp4.clear();
			boost::split(lineTmp4, lineTmp5.at(1), boost::is_any_of("}"));

			if (lineTmp4.at(0) == "exponential")
			{
				if (!CheckDistributionDataForConsistency(EXPONENTIAL, lineTmp2))
					return false;
				(*latencyDescription_).latencyDistribution = EXPONENTIAL;
				lineTmp5.clear();
				lineTmp6.clear();
				boost::algorithm::split_regex(lineTmp5, lineTmp2.at(1), boost::regex ("latencyLambda"));
				if (lineTmp5.size() > 1)
				{
					boost::split(lineTmp6, lineTmp5.at(1), boost::is_any_of("{"));
					lineTmp5.clear();
					boost::split(lineTmp5, lineTmp6.at(1), boost::is_any_of("}"));
					(*latencyDescription_).exponentialLambda = atof(lineTmp5.at(0).c_str());

				}
				else
				{
					cout << "ERROR: lambda not provided in openmsc.msc file for exponential distribution: " << lineTmp2.at(1) << endl;
					return false;
				}
			}
			else if (lineTmp4.at(0) == "pareto")
			{
				if (!CheckDistributionDataForConsistency(PARETO, lineTmp2))
					return false;

				(*latencyDescription_).latencyDistribution = PARETO;
			}
			else if (lineTmp4.at(0) == "uniformReal")
				(*latencyDescription_).latencyDistribution = UNIFORM_REAL;
			else if (lineTmp4.at(0) ==  "uniformInt")
				(*latencyDescription_).latencyDistribution = UNIFORM_INTEGER;
			else if (lineTmp4.at(0) == "gaussian")
				(*latencyDescription_).latencyDistribution = GAUSSIAN;
			else
				(*latencyDescription_).latencyDistribution = LINEAR;
		}
	}

	if (!((*latencyDescription_).latencyDistribution > 0))
	{
		cout << "ERROR: No distribution details given: " << line.at(1) << endl;
		return false;
	}
	return true;
}

bool ReadMsc::CheckDistributionDataForConsistency(LATENCY_DISTRIBUTION dist,
		MSC_LINE_VECTOR line)
{
	MSC_LINE_VECTOR tmp1, tmp2;

	boost::algorithm::split_regex(tmp1, line.at(1), boost::regex ("latencyDist"));

	if (tmp1.size() <= 1) {
		cout << "ERROR: latencyDist information is missing\n";
		return false;
	}

	tmp1.clear();

	if (dist == EXPONENTIAL)
	{
		boost::algorithm::split_regex(tmp1, line.at(1), boost::regex ("latencyLambda"));

		if (tmp1.size() <= 1)
		{
			cout << "ERROR: latencyLambda is missing for exponential distribution: " << line.at(1) << endl;
			return false;
		}

		tmp1.clear();
		boost::algorithm::split_regex(tmp1, line.at(1), boost::regex ("latencyMin"));

		if (tmp1.size() <= 1)
		{
			cout << "ERROR: latencyMin is missing for exponential distribution\n";
			return false;
		}

		tmp1.clear();
		boost::algorithm::split_regex(tmp1, line.at(1), boost::regex ("latencyMax"));

		if (tmp1.size() <= 1)
		{
			cout << "ERROR: latencyMax is missing for exponential distribution\n";
			return false;
		}
	}

	return true;
}

void ReadMsc::CheckNetworkElementIdentifier(NETWORK_ELEMENT networkElement)
{
	if (networkElementsMap.find(networkElement) == networkElementsMap.end())
	{
		networkElementsCounter++;
		networkElementsMap.insert(pair <NETWORK_ELEMENT,IDENTIFIER> (networkElement,networkElementsCounter));
		cout << "CheckNetworkElementIdentifier() New ID: Network Element " << networkElement << " -> " << networkElementsCounter << endl;
	}
}

IDENTIFIER ReadMsc::GetNetworkElementIdentifier(NETWORK_ELEMENT networkElement)
{
	NETWORK_ELEMENTS_MAP_IT it;

	it = networkElementsMap.find(networkElement);

	if (it != networkElementsMap.end())
	{
		return (*it).second;
	}

	return 0;
}

void ReadMsc::CheckPrimitiveNameIdentifier(PRIMITIVE_NAME primitiveName)
{
	if (primitiveNamesMap.find(primitiveName) == primitiveNamesMap.end())
	{
		primitiveNamesCounter++;
		primitiveNamesMap.insert(pair <NETWORK_ELEMENT,IDENTIFIER> (primitiveName,primitiveNamesCounter));
		cout << "CheckPrimitiveNameIdentifier() New ID: Primitive Name " << primitiveName << " -> " << primitiveNamesCounter << endl;
	}
}

IDENTIFIER ReadMsc::GetPrimitiveNameIdentifier(PRIMITIVE_NAME primitiveName)
{
	PRIMITIVE_NAMES_MAP_IT it;

	it = primitiveNamesMap.find(primitiveName);

	if (it != primitiveNamesMap.end())
	{
		return (*it).second;
	}

	return 0;
}

void ReadMsc::CheckProtocolTypeIdentifier(PROTOCOL_TYPE protocolType)
{
	if (protocolTypesMap.find(protocolType) == protocolTypesMap.end())
	{
		protocolTypesCounter++;
		protocolTypesMap.insert(pair <PROTOCOL_TYPE,IDENTIFIER> (protocolType,protocolTypesCounter));
		cout << "CheckProtocolTypeIdentifier() New ID: Protocol Type " << protocolType << " -> " << protocolTypesCounter << endl;
	}
}
void ReadMsc::CheckInformationElementIdentifier(INFORMATION_ELEMENT informationElement)
{
	if (informationElementsMap.find(informationElement) == informationElementsMap.end())
	{
		informationElementsCounter++;
		informationElementsMap.insert(pair <INFORMATION_ELEMENT,IDENTIFIER> (informationElement,informationElementsCounter));
		cout << "CheckInformationElementIdentifier() New ID: Information Element " << informationElement << " -> " << informationElementsCounter << endl;
	}
}

IDENTIFIER ReadMsc::GetProtocolTypeIdentifier(PROTOCOL_TYPE protocolType)
{
	PROTOCOL_TYPES_MAP_IT it;

	it = protocolTypesMap.find(protocolType);

	if (it != protocolTypesMap.end())
	{
		return (*it).second;
	}

	return 0;
}

COMMUNICATION_DESCRIPTION_STRUCT ReadMsc::GetParticularCommunicationDescription (USE_CASE_ID useCaseId,
		int step)
{
	USE_CASE_DESCRIPTION_MAP_IT it;

	it = useCaseDescrMap.find(useCaseId);

	return (*it).second.at(step);
}
IDENTIFIER ReadMsc::TranslateNetworkElement2ID(NETWORK_ELEMENT ne, BS_ID bsId, UE_ID ueId)
{
	NETWORK_ELEMENTS_MAP_IT it;
	// check that NE != (BS && UE)
	if (ne == "BS")
		return bsId*100;
	else if (ne == "UE")
		return (bsId * 100 + ueId);

	it = networkElementsMap.find(ne);

	if (it == networkElementsMap.end())
		return 0;

	return (*it).second;
}
IDENTIFIER ReadMsc::TranslateProtocolType2ID(PROTOCOL_TYPE pt)
{
	PROTOCOL_TYPES_MAP_IT it;

	it = protocolTypesMap.find(pt);

	if (it == protocolTypesMap.end())
		return 0;

	return (*it).second;
}
IDENTIFIER ReadMsc::TranslatePrimitiveName2ID(PRIMITIVE_NAME pn)
{
	PRIMITIVE_NAMES_MAP_IT it;

	it = primitiveNamesMap.find(pn);

	if (it == primitiveNamesMap.end())
		return 0;

	return (*it).second;
}
IDENTIFIER ReadMsc::TranslateInformationElement2ID(INFORMATION_ELEMENT ie)
{
	INFORMATION_ELEMENTS_MAP_IT it;

	it = informationElementsMap.find(ie);

	if (it == informationElementsMap.end())
		return 0;

	return (*it).second;
}
