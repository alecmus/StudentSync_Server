/*
** StudentSync LAN Synchronization Tool: Server
** Copyright (c) 2020
** Alec T. Musasa (alecmus at live dot com),
** Kennedy J.J. Maturure (kenjjmat at outlook dot com)
** 
** Released under the Creative Commons Attribution Non-Commercial
** 2.0 Generic license (CC BY-NC 2.0).
** 
** See accompanying file CC-BY-NC-2.0.txt or copy at [here](https://github.com/alecmus/StudentSync_Server/blob/master/CC-BY-NC-2.0.txt).
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

// STL includes
#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include <liblec/lecui.h>   // for time_stamp()

// for serializing vectors
#include <boost/serialization/vector.hpp>

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#define VERBOSE 0

void log(std::string info) {
    info = liblec::lecui::date::time_stamp() + " " + (info + "\n");
    std::cout << info;
}

struct file {
    std::string filename;
    std::string filedata;
};

// template definition to make file serializable
template <class Archive>
void serialize(Archive& ar, file& data, const unsigned int version) {
    ar& data.filename;
    ar& data.filedata;
}

enum sync_mode { filenames = 1, filelist = 2 };

struct sync_data {
    int mode;
    std::string payload;
};

// serialize files
bool serialize_files(const std::vector<file>& files,
    std::string& serialized, std::string& error) {
    try {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa& files;
        serialized = ss.str();
        return true;
    }
    catch (const std::exception & e) {
        error = e.what();
        return false;
    }
}

// deserialize files
bool deserialize_files(const std::string& serialized,
    std::vector<file>& files, std::string& error) {
    try {
        std::stringstream ss;
        ss << serialized;
        boost::archive::text_iarchive ia(ss);
        ia& files;
        return true;
    }
    catch (const std::exception & e) {
        error = e.what();
        return false;
    }
}

// template definition to make sync_data serializable
template <class Archive>
void serialize(Archive& ar, sync_data& data, const unsigned int version) {
    ar& data.mode;
    ar& data.payload;
}

// serialize sync_data
bool serialize_sync_data(const sync_data& data,
    std::string& serialized, std::string& error) {
    try {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa& data;
        serialized = ss.str();
        return true;
    }
    catch (const std::exception & e) {
        error = e.what();
        return false;
    }
}

// deserialize a sync_data object
bool deserialize_sync_data(const std::string& serialized,
    sync_data& data, std::string& error) {
    try {
        std::stringstream ss;
        ss << serialized;
        boost::archive::text_iarchive ia(ss);
        ia& data;
        return true;
    }
    catch (const std::exception & e) {
        error = e.what();
        return false;
    }
}

// serialize filename list
bool serialize_filename_list(const std::vector<std::string>& filename_list,
    std::string& serialized, std::string& error) {
    try {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa& filename_list;
        serialized = ss.str();
        return true;
    }
    catch (const std::exception & e) {
        error = e.what();
        return false;
    }
}

// deserialize filename list
bool deserialize_filename_list(const std::string& serialized,
    std::vector<std::string>& filename_list,
    std::string& error) {
    try {
        std::stringstream ss;
        ss << serialized;
        boost::archive::text_iarchive ia(ss);
        ia& filename_list;
        return true;
    }
    catch (const std::exception & e) {
        error = e.what();
        return false;
    }
}

std::string process_data_received(const liblec::lecnet::tcp::server::client_address& address,
	const std::string& data_received) {
    std::string error;
    static std::map<std::string, file> consolidated_file_list;  // <K, T> = <filename, file>; list of all the files in the pool
    static std::map<liblec::lecnet::tcp::server::client_address, std::vector<std::string>> client_filename_list;    // list of names of files given clients have

    // deserialize sync_data
    sync_data data;
    if (!deserialize_sync_data(data_received, data, error)) {
        log(error);
        return std::string();
    }

    // get mode
    switch (data.mode) {
    case sync_mode::filenames: {

#if VERBOSE
        log("Receiving list of files from client: " + address);
#endif

        // deserialize filename_list
        std::vector<std::string> filename_list;
        if (!deserialize_filename_list(data.payload, filename_list, error)) {
            log(error);
            return std::string();
        }

#if VERBOSE
        std::string s;
        for (const auto& it : filename_list)
            s += (it + "; ");

        log("Client files are: " + s);
#endif

        // add to client_filename_list map
        client_filename_list[address] = filename_list;

        // check files not in consolidated file list
        std::vector<std::string> missing_filename_list;

        for (const auto& filename : filename_list)
            if (consolidated_file_list.find(filename) == consolidated_file_list.end())
                missing_filename_list.push_back(filename);

        // serialize list of files
        std::string serialized_filename_list;
        if (!serialize_filename_list(missing_filename_list, serialized_filename_list, error)) {
            log(error);
            return std::string();
        }

#if VERBOSE
        if (!missing_filename_list.empty()) {
            s.clear();
            for (const auto& it : missing_filename_list)
                s += (it + "; ");

            std::cout << liblec::lecui::date::time_stamp() + " ";
            printf("\x1B[32m%s\033[0m", "Requesting the following: ");
            std::cout << s << std::endl;
        }
#endif

        // create a sync data object
        sync_data reply_data;
        reply_data.mode = sync_mode::filenames;
        reply_data.payload = serialized_filename_list;

        // serialize the sync_data and send back to client
        std::string serialized_sync_data;
        if (!serialize_sync_data(reply_data, serialized_sync_data, error)) {
            log(error);
            return std::string();
        }

        return serialized_sync_data;
    } break;

    case sync_mode::filelist: {
        // deserialize files
        std::vector<file> files;
        if (deserialize_files(data.payload, files, error)) {
#if VERBOSE
            log("Receiving requested files from client: " + address);
#endif

            // add to consolidated file list
            for (const auto& this_file : files)
                consolidated_file_list[this_file.filename] = this_file;

            // reply client
            return "Files received";
        }
        else {
            // assume client is requesting for files that they are missing
#if VERBOSE
            log("Client requesting possible missing files");
#endif

            std::vector<file> missing_files;

            for (const auto& this_file : consolidated_file_list) {
                const auto this_client_filename_list = client_filename_list.at(address);

                if (std::find(this_client_filename_list.begin(), this_client_filename_list.end(), this_file.first) == this_client_filename_list.end())
                    missing_files.push_back(this_file.second);
            }

            // serialize missing files
            std::string serialized_missing_files;
            if (serialize_files(missing_files, serialized_missing_files, error)) {
                sync_data missing_files_data;
                missing_files_data.mode = sync_mode::filelist;
                missing_files_data.payload = serialized_missing_files;

                // serialize sync_data
                std::string serialized_sync_data;
                if (serialize_sync_data(missing_files_data, serialized_sync_data, error)) {
                    // send to client
                    if (missing_files.empty()) {
#if VERBOSE
                        log("This client has no missing files");
#endif
                    }
                    else
                        log("Sending " + std::to_string(missing_files.size()) + " files to client");

                    return serialized_sync_data;
                }
                else
                    log(error);
            }
            else
                log(error);

            return std::string();
        }
    } break;
    
    default: break;
    }

	// echo server
	return data_received;
}
