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
#include <iostream>

#include "bbexif.hpp"

int main(int argc, char const* argv[]) {
    static std::string const tool_name = "jsexif";
    static std::string const tool_usage = "usage: jsexif jpeg_file [-html]";
    
    bool outputs_html = false;
    if (argc < 2) {
        std::cout << tool_usage << std::endl;
        return 0;
    }
    // Suppress warning: Loop will run at most once...
//    for (auto i = 2; i < argc; ++i) {
    for (auto i = 2; i < argc;) {
        if (std::string(argv[i]).compare("-html") == 0) {
            outputs_html = true;
            break;
        }
        else {
            std::cout << tool_name << ": Illegal option: " << argv[i] << std::endl;
            std::cout << tool_usage << std::endl;
            return -1;
        }
    }
    
    std::string const filepath(argv[1]);
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
        std::cout << tool_name << ": Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
