/*
** StudentSync_Server.cpp
**
** StudentSync LAN Synchronization Tool: Server
** Copyright (c) 2020
** Alec Musasa (alecmus at live dot com),
** Kennedy J.J. Maturure (kenjjmat at outlook dot com)
** 
** Released under the Creative Commons Attribution Non-Commercial
** 2.0 Generic license (CC BY-NC 2.0).
** 
** See accompanying file CC-BY-NC-2.0.txt or copy at
** https://github.com/alecmus/StudentSync_Server/blob/master/CC-BY-NC-2.0.txt
**
*************************************************************************
** Project Details:
**
** National University of Science and Technology
** SCS2206 Computing in Society Group Project
**
** Group members: Alec Musasa, Kennedy J.J. Maturure
**
*/

// STL includes
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>

// liblec network library
#include <liblec/lecnet/udp.h>  // for UDP Multicasting
#include <liblec/lecnet/tcp.h>
#include <liblec/cui.h>

// process data received
#include "process_data_received.h"

void broadcast() {
    // create a broadcast sender object
    liblec::lecnet::udp::broadcast::sender sender(30003);

    const long long cycle = 1200;   // 1.2 seconds

    log("Fetching host IPs and sending broadcast every " + std::to_string(cycle) + "ms ...");

    std::string ips_serialized;

    while (true) {
        // get list of server ips
        std::vector<std::string> ips;
        liblec::lecnet::tcp::get_host_ips(ips);

        // serialize ip list (seperates IPs using the # symbol)
        std::string ips_serialized_new;

        for (const auto& ip : ips)
            ips_serialized_new += (ip + "#");

        if (ips_serialized != ips_serialized_new) {
            std::cout << liblec::cui::date_gen::time_stamp() + " ";
            printf("\x1B[31m%s\033[0m", "IP list updated: ");
            std::string s;
            for (auto ip : ips)
                s += (ip + "; ");
            std::cout << s << std::endl;
        }

        // send serialized ip list
        unsigned long actual_count = 0;
        std::string error;
        if (!sender.send(ips_serialized_new, 1, 0, actual_count, error))
            std::cout << "Error: " << error << std::endl;

        ips_serialized = ips_serialized_new;

        // wait for as long as the cycle
        std::this_thread::sleep_for(std::chrono::milliseconds(cycle));
    }
}

class server_async : public liblec::lecnet::tcp::server_async {
public:
    void log(const std::string& time_stamp,
        const std::string& event) override {
        std::cout << time_stamp + " " + event + "\n";
    };

    std::string on_receive(const client_address& address,
        const std::string& data_received) override {
        return process_data_received(address, data_received);
    };
};

int main() {
    std::cout << "\n**********************************************\n";
    printf("\x1B[32m%s\033[0m", "StudentSync Server version 1.0.0.0\n");
    std::cout << "**********************************************\n\n";

    // create broadcast thread
    log("Creating broadcast thread ...");
    std::thread broadcast_thread(broadcast);

    // configure server parameters
    liblec::lecnet::tcp::server::server_params params;
    params.port = 55553;
    params.magic_number = 16;
    params.max_clients = 200;

    // create tcp/ip server object
    server_async server;

    // start the server
    if (server.start(params)) {
        while (server.starting())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        broadcast_thread.join();
    }
    else
        std::cout << "Server failed to start!\n";

    return 0;
}
