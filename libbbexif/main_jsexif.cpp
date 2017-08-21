//
//  main_jsexif.cpp
//  libbbexif
//
//  Created by OTAKE Takayoshi on 2017/08/18.
//  Copyright © 2017 OTAKE Takayoshi. All rights reserved.
//

// [C++14]

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <sstream>

#include "bbexif.hpp"

#define COMMAND_NAME "jsexif"

struct lines {
    std::string _str;
    lines(std::vector<std::string> lines)
    : _str([&]() {
        std::stringstream ss;
        for (auto const& line: lines) {
            ss << line << std::endl;
        }
        return ss.str();
    }()) {}
    std::string const& str() const { return _str; }
};

int jsexif(std::list<std::string>& args);
int jsexif_read(std::list<std::string>& args);

void show_jsexif_version() {
    std::cout << "jsexif version 1.0" << std::endl;
}

void show_jsexif_help() {
    std::cout << lines({
        "Usage: " COMMAND_NAME " [options] <subcommands> ...",
        "",
        "Options:",
        "  -h, --help  Show this help message and exit",
        "  --version   Show the jsexif version",
        "",
        "Subcommands:",
        "  read  Show the exif tags as json",
    }).str() << std::endl;
}

void show_jsexif_read_help() {
    std::cout << lines({
        "Usage: " COMMAND_NAME " read <jpeg_file> [options]",
        "",
        "Options:",
        "  --html  Output sample html displays exif json",
    }).str() << std::endl;
}

int jsexif(std::list<std::string>& args) {
    if (args.size() == 0) {
        show_jsexif_help();
        return 0;
    }
    
    // [options]
    if (args.front().compare("-help") == 0 || args.front().compare("--help") == 0) {
        show_jsexif_help();
        return 0;
    }
    else if (args.front().compare("--version") == 0) {
        show_jsexif_version();
        return 0;
    }
    
    // <subcommands>
    auto subcommand = args.front();
    args.pop_front();
    if (subcommand.compare("read") == 0) {
        return jsexif_read(args);
    }
    else {
        show_jsexif_help();
        return 0;
    }
    
    return 0;
}

int jsexif_read(std::list<std::string>& args) {
    if (args.size() == 0) {
        show_jsexif_read_help();
        return 0;
    }
    
    auto filepath = args.front();
    args.pop_front();
    
    bool outputs_html = false;
    for (auto const& option: args) {
        if (option.compare("--html") == 0) {
            outputs_html = true;
        }
        else {
            std::cout << COMMAND_NAME << ": Illegal option: " << option << std::endl;
            show_jsexif_help();
            return 0;
        }
    }
    
    try {
        auto exif = bbexif::read_exif(filepath);
        if (outputs_html) {
            std::cout << "<!DOCTYPE html><html><body><script>" << std::endl;
            std::cout << "var exif = " << bb::stringify(bb::make_json(exif), 0, 2) << std::endl;
            std::cout << "document.write('<pre>')" << std::endl;
            std::cout << "document.write(JSON.stringify(exif, null, 2))" << std::endl;
            std::cout << "document.write('</pre>')" << std::endl;
            std::cout << "</script></body></html>" << std::endl;
        }
        else {
            std::cout << bb::stringify(bb::make_json(exif), 0, 2) << std::endl;
        }
    }
    catch (std::exception const& e) {
        std::cout << COMMAND_NAME << ": Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}

int main(int argc, char const* argv[]) {
    std::list<std::string> args;
    for (auto i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return jsexif(args);
}
