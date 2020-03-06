/*
** StudentSync LAN Synchronization Tool: Server
**
** This code may not be copied, modified or distributed without the
** express written permission of the author(s). Any violation shall
** be prosecuted to the maximum extent possible under law.
**
*************************************************************************
** Project Details:
**
** National University of Science and Technology
** SCS2206 Computing in Society Group Project
**
** Group members: Alec T. Musasa, Kennedy J.J. Maturure
**
*/

#include "process_data_received.h"

#include <iostream>

std::string process_data_received(const liblec::lecnet::tcp::server::client_address& address,
	const std::string& data_received) {
	std::cout << "Data received from " + address + ": " + data_received + "\n";

	// echo server
	return data_received;
}
