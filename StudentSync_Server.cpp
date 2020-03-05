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
#include <liblec/lecui.h>

void log(std::string info) {
    info = liblec::lecui::date::time_stamp() + " " + (info + "\n");
    std::cout << info;
}

void multicast_thread() {
    // create a multicast sender object
    liblec::lecnet::udp::multicast::sender sd(30003, "239.255.0.3");

    const long long cycle = 1200;   // 1.2 seconds

    log("Fetching host IPs and sending multicast every " + std::to_string(cycle) + "ms ...");

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
            std::cout << liblec::lecui::date::time_stamp() + " ";
            printf("\x1B[31m%s\033[0m", "IP list updated: ");
            std::string s;
            for (auto ip : ips)
                s += (ip + "; ");
            std::cout << s << std::endl;
        }

        // send serialized ip list
        unsigned long actual_count = 0;
        std::string error;
        if (!sd.send(ips_serialized_new, 1, 0, actual_count, error))
            std::cout << "Error: " << error << std::endl;

        ips_serialized = ips_serialized_new;

        // wait for as long as the cycle
        std::this_thread::sleep_for(std::chrono::milliseconds(cycle));
    }
}

class server : public liblec::lecnet::tcp::server_async {
public:
    void log(const std::string& time_stamp,
        const std::string& event) override {
        std::cout << time_stamp + " " + event + "\n";
    };

    std::string on_receive(const client_address& address,
        const std::string& data_received) {
        std::cout << "Data received from " + address + ": " + data_received + "\n";

        // echo server
        return data_received;
    };

private:

};

int main() {
    std::cout << "\n**********************************************\n";
    printf("\x1B[32m%s\033[0m", "StudentSync Server version 1.0.0.0\n");
    std::cout << "**********************************************\n\n";

    // create multicast thread
    log("Creating multicast thread ...");
    std::thread t(multicast_thread);

    // configure server parameters
    liblec::lecnet::tcp::server::server_params params;
    params.port = 55553;
    params.magic_number = 16;
    params.max_clients = 200;

    // create tcp/ip server object
    server s;

    // start the server
    if (s.start(params)) {
        while (s.starting())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        t.join();
    }
    else
        std::cout << "Server failed to start!\n";

    return 0;
}
