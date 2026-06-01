#pragma once

#include <string>

struct Config {
    int         version     = 1;;
    std::string server_name = "Dedicated Server";
    std::string server_ip   = "play.example.com";
    int         server_port = 19132;
}