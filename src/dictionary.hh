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

#include "typedef.hh"
#include <log4cxx/logger.h>
class Dictionary {
public:
	/**
	 * Extend the networkElementsDict with the network element name and its corresponding numeric identifier
	 * @param ne The network element
	 * @param id The numeric identifier
	 * @return void
	 */
	void WriteNetworkElement(NETWORK_ELEMENT ne, IDENTIFIER id);
	/**
	 * Extend the protocolTypesDict with the protocol type name and its corresponding numeric identifier
	 * @param ne The protocol type
	 * @param id The numeric identifier
	 * @return void
	 */
	void WriteProtocolType(PROTOCOL_TYPE pt, IDENTIFIER id);
	/**
	 * Extend the primitiveNamesDict with the protocol type name and its corresponding numeric identifier
	 * @param ne The primitive name
	 * @param id The numeric identifier
	 * @return void
	 */
	void WritePrimitiveName(PRIMITIVE_NAME pn, IDENTIFIER id);
	/**
	 * Extend the informationElementsDict with the protocol type name and its corresponding numeric identifier
	 * @param ne The information element
	 * @param id The numeric identifier
	 * @return void
	 */
	void WriteInformationElement(INFORMATION_ELEMENT ie, IDENTIFIER id);
	/**
	 * Initilising all dictionaries
	 * @return void
	 */
	void Init();
	/**
	 * Initialising logging in ReadMsc class
	 * @param l Pointer to LoggerPtr class
	 */
	void InitLog(log4cxx::LoggerPtr l);
private:
	DICTIONARY_FILE_NAME networkElementsDict,	/** dictionary name for NEs & their corresponding integer representations */
		protocolTypesDict,						/** dictionary name for protocol types & their corresponding integer representations */
		primitiveNamesDict,						/** dictionary name for primitive names & their corresponding integer representations */
		informationElementsDict;				/** dictionary name for IEs & their corresponding integer representations */
	log4cxx::LoggerPtr logger;	/** Pointer to LoggerPtr class */
};
