/**
 * @author Sebastian Robitzsch <srobitzsch@gmail.com>
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

#include "dictionary.hh"
#include <fstream>

void Dictionary::Init()
{
	ofstream dict;
	networkElementsDict = "dict-networkElements.tsv";
	protocolTypesDict = "dict-protocolTypes.tsv";
	primitiveNamesDict = "dict-primitiveNames.tsv";
	informationElementsDict = "dict-informationElements.tsv";

	dict.open(networkElementsDict, ios::trunc);
	dict << "# Automatically generated file by OpenMSC" << endl;
	dict.close();
	dict.open(protocolTypesDict, ios::trunc);
	dict << "# Automatically generated file by OpenMSC" << endl;
	dict.close();
	dict.open(primitiveNamesDict, ios::trunc);
	dict << "# Automatically generated file by OpenMSC" << endl;
	dict.close();
	dict.open(informationElementsDict, ios::trunc);
	dict << "# Automatically generated file by OpenMSC" << endl;
	dict.close();
}
void Dictionary::InitLog(log4cxx::LoggerPtr l)
{
	logger = l;
}
void Dictionary::WriteNetworkElement(NETWORK_ELEMENT ne, IDENTIFIER id)
{
	ofstream dict;

	dict.open (networkElementsDict, ios::app);
	dict << ne << "\t" << id << endl;
	dict.close();
	LOG4CXX_DEBUG(logger, "Dictionary '" << networkElementsDict << "' extended with network element " << ne << " and ID " << id);
}
void Dictionary::WriteProtocolType(PROTOCOL_TYPE pt, IDENTIFIER id)
{
	ofstream dict;

	dict.open (protocolTypesDict, ios::app);
	dict << pt << "\t" << id << endl;
	dict.close();
	LOG4CXX_DEBUG(logger, "Dictionary '" << protocolTypesDict << "' extended with protocol type " << pt << " and ID " << id);
}
void Dictionary::WritePrimitiveName(PRIMITIVE_NAME pn, IDENTIFIER id)
{
	ofstream dict;

	dict.open (primitiveNamesDict, ios::app);
	dict << pn << "\t" << id << endl;
	dict.close();
	LOG4CXX_DEBUG(logger, "Dictionary '" << primitiveNamesDict << "' extended with primitive name " << pn << " and ID " << id);
}
void Dictionary::WriteInformationElement(INFORMATION_ELEMENT ie, IDENTIFIER id)
{
	ofstream dict;

	dict.open (informationElementsDict, ios::app);
	dict << ie << "\t" << id << endl;
	dict.close();
	LOG4CXX_DEBUG(logger, "Dictionary '" << informationElementsDict << "' extended with information element " << ie << " and ID " << id);
}
