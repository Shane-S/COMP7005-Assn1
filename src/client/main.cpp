/* ========================================================================
   $File: main.cpp $
   $Program: $
   $Developer: Shane Spoor $
   $Created On: 2016/09/22 $
   $Description: $
   $     Starts the client side of the application
   $Revisions: $
   ======================================================================== */
#include <iostream>
#include <string>
#include <vector>
#include <client/client.h>
#include <util/packet.hpp>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

/* ========================================================================
   $ FUNCTION
   $ Name: parse_command $
   $ Prototype: bool parse_command(std::string& command, packet_type& op, std::string& filename) { $
   $ Params: 
   $    command: The command from the user $
   $    op: The packet type
   $    filename: The name of the file
   $ Description:  $
   $    Parses commands from the user to send to the client.
   ======================================================================== */
bool parse_command(std::string& command, packet_type& op, std::string& filename) {
    std::vector<std::string> command_parts;
    boost::split(command_parts, command, boost::is_any_of(" "));

    if(command_parts.size() < 2 || (command_parts[0] != "GET" && command_parts[0] != "SEND")) {
        return false;
    } else {
        // Figure out the operation
        op = command_parts[0] == "GET" ? GET : SEND;

        // Remove GET/SEND from the string and copy the remainder of the string into filename
        command_parts.erase(command_parts.begin(), command_parts.begin() + 1);
        filename = boost::algorithm::join(command_parts, "");

        return true;
    }
}

/* ========================================================================
   $ FUNCTION
   $ Name: main $
   $ Prototype: int main(int argc, char** argv) { $
   $ Params: 
   $    argc: The number of inputs from the user $
   $    argv: The inputs from the user
   $ Description:  $
   $    Starts the client application
   ======================================================================== */
int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "usage: " << argv[0] << " [host name] [file storage path]" << std::endl;
        return 1;
    }

    boost::asio::io_service service;
    std::string host_name(argv[1]);
    std::string storage_path(argv[2]);

    try {
        client c(service, host_name, storage_path);
        std::cout << std::endl;

        std::cout << "File names are relative to the storage path supplied." << std::endl;
        std::cout << "Type SEND [filename] to send a file to the server." << std::endl;
        std::cout << "Type GET [filename] to get a file from the server." << std::endl;
        std::cout << "Type Ctrl + D to quit" << std::endl;
        std::cout << std::endl;

        while(true) {
            std::string command;
            std::cout << "Enter a command: ";
            std::getline(std::cin, command);

            // Check whether they've quit
            if(!std::cin) {
                break;
            }

            packet_type op;
            std::string filename;
            if(!parse_command(command, op, filename)) {
                std::cout << "Command format: SEND|GET [filename]" << std::endl;
            } else {
                if(op == SEND) {
                    c.send(filename);
                } else {
                    c.get(filename);
                }
            }

        }
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "See ya!" << std::endl;

    return 0;
}
