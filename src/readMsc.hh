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

#include "dictionary.hh"
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
	/**
	 * Obtain the number of communication descriptions for a particular use-case ID
	 * @param useCaseId The use-case ID for which the length should be determined
	 * @return The number of communication descriptions. If return value is zero, the given use-case ID did not exist.
	 */
	int GetMscLength(USE_CASE_ID useCaseId);
	/**
	 * Obtain the number of use cases defined by the user
	 * @return Integer representation of the number of use-cases stored in useCaseDescrMap
	 */
	int GetNumOfUseCases();
	/**
	 * Obtain the probability for a given use-case ID
	 * @param useCaseId The integer number for the use-case ID
	 * @return The probability for the given use-case ID
	 */
	USE_CASE_ID GetUseCaseId4Probability(PROBABILITY p);
	/**
	 * Calculate the value for an information element different from UE_ID or BS_ID.
	 * @param ie The information element
	 * @return The max 3 digit long integer value
	 */
	int GetIeValue(INFORMATION_ELEMENT ie);
	/**
	 * Looking up this communication descriptor whether or not this is a periodic message
	 * @param step Indicating the step of this MSC
	 */
	bool GetPeriodicCommunicationDescriptorFlag(USE_CASE_ID useCaseId, int step);
	/**
	 * Obtain a copy of a particular communication description struct
	 * @param useCaseId The use-case ID for which the data should be looked up
	 * @param step The step within the use-case which should be returned
	 * @return A copy of the communication description with type COMMUNICATION_DESCRIPTION_STRUCT
	 */
	COMMUNICATION_DESCRIPTION_STRUCT GetParticularCommunicationDescription (
			USE_CASE_ID useCaseId,
			int step );
	/**
	 * Translating the network element name (string) into a unique numeric representation using the networkElementMap.
	 * @param ne The network element which should be translated
	 * @param bsId The base-station ID in case the NE is a base-station in which case the base-station ID generated in openmsc.cc is used
	 * @param ueId The user equipment ID in case the NE is a user equipment in which case the user equipment ID generated in openmsc.cc is used
	 * @return A unique identifier for the given network element
	 */
	IDENTIFIER TranslateNetworkElement2ID(
			NETWORK_ELEMENT ne,
			BS_ID bsId,
			UE_ID ueId );
	/**
	 * Translating the protocol type name (string) into a unique numeric representation using the protocolTypeMap.
	 * @param pt The protocol type name which should be translated
	 * @return A unique identifier for the given protocol type
	 */
	IDENTIFIER TranslateProtocolType2ID(PROTOCOL_TYPE pt);
	/**
	 * Translating the primitive name (string) into a unique numeric representation using the primitiveNameMap.
	 * @param pn The primitive name which should be translated
	 * @return A unique identifier for the given primitive name
	 */
	IDENTIFIER TranslatePrimitiveName2ID(PRIMITIVE_NAME pn);
	/**
	 * Translating the information element name (string) into a unique numeric representation using the informationElementMap
	 * @param ie The information element name which should be translated
	 * @return A unique identifier for the given information element name
	 */
	IDENTIFIER TranslateInformationElement2ID(INFORMATION_ELEMENT ie);
	/**
	 * Keep pointer to Dictionary class and its previous initilisation in openmsc.cc
	 * @param dict_ Pointer to Dictionary class
	 */
	void EstablishDictConnection(Dictionary *dict_);
	/**
	 * Initialising logging in ReadMsc class
	 * @param l Pointer to LoggerPtr class
	 */
	void InitLog(log4cxx::LoggerPtr l);
	/**
	 * This function copies the number of UEs attached to a BS and the number of BSs in the network to the private variables numOfUesPerBs and numOfBss
	 * @param uesPerBs_ Number of UEs attached to a BS
	 * @param bss_ The total number of BSs in the network
	 */
	void AddConfig(int *uesPerBs_, int *bss_);
	/**
	 * This function adds an IE description from openmsc.cfg to the INFORMATION_ELEMENT_DESCRIPTION_MAP
	 * @param ieDescrStruct_ Pointer to the pair holding all the information for a particular IE
	 */
	void AddInformationElementDescription(INFORMATION_ELEMENT_DESCRIPTION_PAIR ieDescrPair);
private:
	/**
	 * \brief add communication description
	 *
	 * Adding a single communication to the map of use-cases
	 *
	 * @param uc Use-case identifier
	 * @param src Source network element
	 * @param dst Destination network element
	 * @param protType Message type
	 * @param primName Primitive name
	 * @param infElements Vector of information elements contained in this message
	 * @param latencyDescription Latency desription for this communication descriptor
	 * @param periodicFlag Flag indicating if this communication descriptor is a periodic one
	 * @return boolean indicating successful/unsuccessful processing of the given data
	 */
	bool AddCommunicationDescription (
			USE_CASE_ID uc,
			NETWORK_ELEMENT src,
			NETWORK_ELEMENT dst,
			PROTOCOL_TYPE protType,
			PRIMITIVE_NAME primName,
			INFORMATION_ELEMENT_VECTOR infElements,
			DISTRIBUTION_DEFINITION_STRUCT latencyDescription,
			bool periodicFlag);

	/**
	 * \brief Extracting communication description from a given line
	 *
	 * @param line The line which will be parsed
	 * @param src_ Pointer into which the source network element will be written
	 * @param dst_ Pointer into which the destination network element will be written
	 * @param protocolType_ Pointer into which the protocol type will be written
	 * @param primitiveName_ Pointer into which the protocol name will be written
	 * @param informationElements_ Pointer into which the information elements will be written
	 * @param latencyDescription_ Pointer into which the latency distribution information will be written
	 * @return boolean indicating whether or not the parsing was successful
	 */
	bool ExtractDataFromLine(
			MSC_LINE_VECTOR line,
			NETWORK_ELEMENT *src_,
			NETWORK_ELEMENT *dst_,
			PROTOCOL_TYPE *protocolType_,
			PRIMITIVE_NAME *primitiveName_,
			INFORMATION_ELEMENT_VECTOR *informationElements_,
			DISTRIBUTION_DEFINITION_STRUCT *latencyDescription_ );
	/**
	 * \brief Function to parse latency data from MSC line and extract the value for given element
	 * @param line The full line which comprises all latency information
	 * @param element The latency element for which its value should be looked up
	 * @return The value for the given latency element
	 */
	const char * GetLatencyValue(
			string line,
			string element );
	/**
	 * \brief Distribution details from openmsc.msc are being checked for consistency
	 * @param dist The distribution which should be checked for completeness
	 * @param line The line which holds all the given information
	 * @return Boolean indicating whether or not the given data was correct
	 */
	bool CheckDistributionDataForConsistency(
			DISTRIBUTION dist,
			string line );
	/**
	 * This function checks if the IE stored in COMMUNICATION_DESCRIPTION_STRUCT > informationElements
	 * has been specified and properly parsed and stored in INFORMATION_ELEMENT_DESCRIPTION_STRUCT.
	 */
	bool CheckInformationElementIntegrity(INFORMATION_ELEMENT ie);
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
	 * Check if given information element is known. If not, create a new unique identifier for this IE.
	 * @param informationElement The IE which should be checked
	 */
	void CheckInformationElementIdentifier(INFORMATION_ELEMENT informationElement);
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
	INFORMATION_ELEMENT_DESCRIPTION_MAP ieDescrMap;	/** map holding IE information (distributions / ranges) from openmsc.cfg file */
	USE_CASE_PROBABILITY_MAP useCaseProbabilityMap; /** TODO */
	NETWORK_ELEMENTS_MAP networkElementsMap;	/** TODO */
	PRIMITIVE_NAMES_MAP primitiveNamesMap;		/** TODO */
	PROTOCOL_TYPES_MAP protocolTypesMap;		/** TODO */
	INFORMATION_ELEMENTS_MAP informationElementsMap;	/** TODO */
	NETWORK_ELEMENTS_COUNTER networkElementsCounter;	/** TODO */
	PROTOCOL_TYPES_COUNTER protocolTypesCounter;	/** TODO */
	PRIMITIVE_NAMES_COUNTER primitiveNamesCounter;	/** TODO */
	INFORMATION_ELEMENTS_COUNTER informationElementsCounter;	/** TODO */
	Dictionary *dictionary_;	/** Pointer to Dictionary class */
	log4cxx::LoggerPtr logger;	/** Pointer to LoggerPtr class */
	int *numOfUesPerBs_;			/** Number of UEs attached to each BS */
	int *numOfBss_;				/** Number of BSs in the network */
	base_generator_type gen;
};
