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

#include "readMsc.hh"

/**
 * \class EventIdGenerator
 * This class generates EventIDs based on the MSC.
 */
class EventIdGenerator {
public:
	/**
	 * Initilising LatencyCalculator class with obtaining the pointer to the ReadMsc class which holds all the latency settings provided by the user.
	 * @param rMsc_ Pointer to ReadMsc class
	 * @return void
	 */
	void Init(ReadMsc *rMsc_);
	/**
	 * Determine the use-case ID based on the probabilities defined in openmsc.msc
	 * @return The use-case ID of type USE_CASE_ID
	 */
	USE_CASE_ID DetermineUseCaseId();
	/**
	 * For a given use-case and step within this use-case, this function generates the corresponding EventId.
	 *
	 * @param useCaseId The use-case ID for which the data should be looked up
	 * @param step The step within the use-case which should be returned
	 * @param bsId The identifier of the BS to which the UE is connected to
	 * @param ueId The identifier of the UE which is sending
	 * @return Vector of EventIDs of a particular communication description
	 */
	EVENT_ID_VECTOR GetEventIdForComDescr(
			USE_CASE_ID useCaseId,
			int step,
			BS_ID bsId,
			UE_ID ueId );
	/**
	 * @param ucId The use-case ID
	 * @param step The communication description steps within this use-case
	 * @return The latency with which the message arrive at the destination
	 */
	TIME CalculateLatency(USE_CASE_ID ucId, int step);
	/**
	 * Initialising logging in ReadMsc class
	 * @param l Pointer to LoggerPtr class
	 */
	void InitLog(log4cxx::LoggerPtr l);
private:
	ReadMsc *readMsc_;
	log4cxx::LoggerPtr logger;	/** Pointer to LoggerPtr class */
};
