//
// blocking_udp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/**
 * @file
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

#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;
using namespace std;
enum { max_length = 1024 };

void server(boost::asio::io_service& io_service, unsigned short port)
{
	udp::socket sock(io_service, udp::endpoint(udp::v4(), port));
	for (;;)
	{
		int sizeSource = 5,
			sizeDestination = 5,
			sizeProtocolType = 2,
			sizePrimitiveName = 2,
			informationElement = 2,
			informationElementValue = 3,
			start,
			end;
		char data[max_length];
		udp::endpoint sender_endpoint;
		size_t length = sock.receive_from(boost::asio::buffer(data, max_length), sender_endpoint);
		cout << "#######################\nReceived msg from " << sender_endpoint.address() << ":\n";
		start = 0;
		end = sizeSource;
		cout << "Source (" << end - start << "):\t\t";
		for (int i = start; i < end; i++)
			cout << data[i];
		cout << endl;
		start += sizeSource;
		end += sizeDestination;
		cout << "Destination (" << end - start << "):\t";
		for (int i = start; i < end; i++)
			cout << data[i];
		cout << endl;
		start += sizeDestination;
		end += sizeProtocolType;
		cout << "Protocol Type (" << end - start << "):\t";
		for (int i = start; i < end; i++)
			cout <<  data[i];
		cout << endl;
		start += sizeProtocolType;
		end += sizePrimitiveName;
		cout << "Primitive Name (" << end - start << "):\t";
		for (int i = start; i < end; i++)
			cout << data[i];
		cout << endl;
		start += sizePrimitiveName;
		end += informationElement;
		cout << "Information Element (" << end - start << "):";
		for (int i = start; i < end; i++)
			cout << data[i];
		cout << endl;
		start += informationElement;
		end += informationElementValue;
		cout << "IE Value (" << end - start << "):\t\t";
		for (int i = start; i < end; i++)
			cout << data[i];
		cout << endl;
		//sock.send_to(boost::asio::buffer(data, length), sender_endpoint);
	}
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: blocking_udp_echo_server <port>\n";
			return 1;
		}

		boost::asio::io_service io_service;

		using namespace std; // For atoi.
		server(io_service, atoi(argv[1]));
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
