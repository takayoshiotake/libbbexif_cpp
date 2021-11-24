//
//  debug.hpp
//
//  Created by OTAKE Takayoshi on 2017/08/19.
//  Copyright © 2017 OTAKE Takayoshi. All rights reserved.
//

#pragma once

// [C++14]

#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <iomanip>

namespace bb {
    inline std::string make_log_message(char const* format, ...) {
        auto time_point = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(time_point);
        auto time_usec = std::chrono::duration_cast<std::chrono::microseconds>(time_point - std::chrono::time_point_cast<std::chrono::seconds>(time_point)).count();
        
        char buf[1024];
        va_list arg;
        va_start(arg, format);
        vsnprintf(buf, sizeof(buf), format, arg);
        va_end(arg);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%F %T.") << std::setw(6) << std::setfill('0') << time_usec << " " << buf;
        return ss.str();
    }
    
#ifdef DEBUG
    inline std::string make_trace_message(char const* file, int line, char const* format, ...) {
        char buf[1024];
        va_list arg;
        va_start(arg, format);
        int pos = vsnprintf(buf, sizeof(buf), format, arg);
        snprintf(&buf[pos], sizeof(buf) - pos, "\n    at %s:%d", file, line);
        va_end(arg);
        return buf;
    }
#else
    inline std::string make_trace_message(char const* format, ...) {
        char buf[1024];
        va_list arg;
        va_start(arg, format);
        vsnprintf(buf, sizeof(buf), format, arg);
        va_end(arg);
        return buf;
    }
#endif
}

#ifdef DEBUG
#define bb_trace_message(format, ...) [&](){\
    return bb::make_trace_message(__FILE__, __LINE__, format, ##__VA_ARGS__);\
}()
#else
#define bb_trace_message(format, ...) [&](){\
    return bb::make_trace_message(format, ##__VA_ARGS__);\
}()
#endif
