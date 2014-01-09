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

/**
 * Class for parsing MSCgen configuration file
 */
class ReadMsc {
public:
	/**
	 * \brief Read in MSC config file
	 *
	 * \return integer return value indicating success (0) or failure (> 0)
	 */
	int ReadMscConfigFile ();
private:
	/**
	 * \brief add communication description
	 *
	 * Adding a single communication to the map of use-cases
	 *
	 * @param uc Pointer to the use-case identifier
	 * @param src Pointer to the source network element descriptor
	 * @param dst Pointer to the destination network element descriptor
	 * @param mt Pointer to the message type descriptor
	 * @param ies Pointer to a vector of information elements contained in this message
	 * @return boolean indicating successful/unsuccessful processing of the given data
	 */
	bool AddCommunicationDescription (USE_CASE_ID uc,
			NETWORK_ELEMENT src,
			NETWORK_ELEMENT dst,
			PROTOCOL_TYPE protType,
			PRIMITIVE_NAME primName,
			INFORMATION_ELEMENT_VECTOR infElements,
			LATENCY_DESCRIPTION_STRUCT latencyDescription );

	/**
	 * \brief Extracting communication description from a given line
	 *
	 * @param line The line which will be parsed
	 * @param src Pointer into which the source network element will be written
	 * @param dst Pointer into which the destination network element will be written
	 * @param protocolType Pointer into which the protocol type will be written
	 * @param primitiveName Pointer into which the protocol name will be written
	 * @param informationElements Pointer into which the information elements will be written
	 * @param latencyDescription Pointer into which the latency distribution information will be written
	 * @return boolean indicating whether or not the parsing was successful
	 */
	bool ExtractDataFromLine(MSC_LINE_VECTOR line,
			NETWORK_ELEMENT *src,
			NETWORK_ELEMENT *dst,
			PROTOCOL_TYPE *protocolType,
			PRIMITIVE_NAME *primtiveName,
			INFORMATION_ELEMENT_VECTOR *informationElements,
			LATENCY_DESCRIPTION_STRUCT *latencyDescription);

	/**
	 * \brief Distribution details from openmsc.msc are being checked for consistency
	 * @param dist The distribution which should be checked for completeness
	 * @param line The line which holds all the given information
	 * @return Boolean indicating whether or not the given data was correct
	 */
	bool CheckDistributionDataForConsistency(LATENCY_DISTRIBUTION dist,
			MSC_LINE_VECTOR line);

	/**
	 * Check if given network element is known. If not, create a new unique identifier for this network element.
	 * @param networkElement The network element which should be checked
	 */
	void CheckNetworkElementIdentifier(NETWORK_ELEMENT networkElement);
	/**
	 * Check if given primitive name is known. If not, create a new unique identifier for this primitive name.
	 * @param primitiveName The primitive name which should be checked
	 */
	void CheckPrimitiveNameIdentifier(PRIMITIVE_NAME primitiveName);
	/**
	 * Check if given protocol type is known. If not, create a new unique identifier for this protocol type.
	 * @param protocolType The protocol type which should be checked
	 */
	void CheckProtocolTypeIdentifier(PROTOCOL_TYPE protocolType);
	/**
	 * Obtain the unique identifier for the given network element.
	 * @param networkElement The network element for which the numerical identifier should be obtained
	 * @return Numerical identifier is returned if it exists in NETWORK_ELEMENTS_MAP. If it does not exist, the function returns 0.
	 */
	IDENTIFIER GetNetworkElementIdentifier(NETWORK_ELEMENT networkElement);
	/**
	 * Obtain the unique identifier for the given primitive name.
	 * @param primitiveName The primitive name for which the numerical identifier should be obtained
	 * @return Numerical identifier is returned if it exists in PRIMITIVE_NAME_MAP. If it does not exist, the function returns 0.
	 */
	IDENTIFIER GetPrimitiveNameIdentifier(PRIMITIVE_NAME primitiveName);
	/**
	 * Obtain the unique identifier for the given protocol type.
	 * @param protocolType The protocol type for which the numerical identifier should be obtained
	 * @return Numerical identifier is returned if it exists in PROTOCOL_TYPE_MAP. If it does not exist, the function returns 0.
	 */
	IDENTIFIER GetProtocolTypeIdentifier(PROTOCOL_TYPE protocolType);

	USE_CASE_DESCRIPTION_MAP useCaseDescrMap; /** map initialiser holding the use-case ID and a vector of all communications*/
	NETWORK_ELEMENTS_MAP networkElementsMap;
	PRIMITIVE_NAMES_MAP primitiveNamesMap;
	PROTOCOL_TYPES_MAP protocolTypesMap;
	NETWORK_ELEMENTS_COUNTER networkElementsCounter;
	PROTOCOL_TYPES_COUNTER protocolTypesCounter;
	PRIMITIVE_NAMES_COUNTER primitiveNamesCounter;
};
